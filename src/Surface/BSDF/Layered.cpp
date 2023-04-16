#include "Core/BSDF.h"
#include "Utils/Error.h"

float transmittance(float z0, float z1, Vec3f w) {
    return glm::exp(-glm::abs((z0 - z1) / w.z));
}

Spectrum LayeredBSDF::bsdf(const SurfaceIntr& intr, TransportMode mode) const {
    if (!top && !bottom) {
        return Spectrum(0.f);
    }
    else if (!top) {
        return bottom->bsdf(intr, mode);
    }
    else {
        return top->bsdf(intr, mode);
    }
    return Spectrum(0.f);
}

float LayeredBSDF::pdf(const SurfaceIntr& intr, TransportMode mode) const {
    if (!top && !bottom) {
        return 0;
    }
    else if (!top) {
        return bottom->pdf(intr, mode);
    }
    else {
        return top->pdf(intr, mode);
    }
    return 0.f;
}

std::optional<BSDFSample> LayeredBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) const {
    if (!top && !bottom) {
        return std::nullopt;
    }
    else if (!top) {
        return bottom->sample(intr, u, mode);
    }
    else {
        return top->sample(intr, u, mode);
    }
    
    auto wo = intr.wo;
    bool entTop = wo.z > 0;

    BSDF* ent = entTop ? top : bottom;
    BSDF* oth = entTop ? bottom : top;

    auto ins = ent->sample(SurfaceIntr(wo, intr.uv, intr.sampler), u, mode);

    if (!ins || ins->pdf < 1e-8f || ins->dir.z == 0) {
        return std::nullopt;
    }

    if (Math::sameHemisphere(wo, ins->dir)) {
        return ins;
    }

    return std::nullopt;
}

struct PathVertex {
    int layer;
    Vec3f wGiven, wSampled;
    Spectrum fRegular, fAdjoint;
    float pdfRegular, pdfAdjoint;
};

std::optional<BSDFSample> generatePath(
    //const SurfaceIntr& intr,
    Vec3f wGiven,
    Vec2f uv,
    const std::vector<BSDF*>& interfaces,
    const std::vector<Texture3fPtr>& normalMaps,
    Sampler* sampler,
    int maxDepth,
    TransportMode mode,
    std::vector<PathVertex>* path = nullptr
) {
    int topLayer = (wGiven.z > 0) ? 0 : interfaces.size() - 1;
    int bottomLayer = (topLayer == 0) ? interfaces.size() - 1 : 0;
    int layer = topLayer;
    bool goingDown = layer == 0;

    BSDFSample bSample;
    Spectrum throughput(1.f);
    float pdf = 1.f;

    Spectrum bsdf0;
    float pdf0;

    for (int depth = 0; depth < maxDepth; depth++) {
        if (layer < 0 || layer >= interfaces.size()) {
            break;
        }
        PathVertex vertex;

        BSDF* layerBSDF = interfaces[layer];
        Vec3f norm = normalMaps[layer] ? (normalMaps[layer]->get(uv.x, uv.y) * 2.f - 1.f) : Vec3f(0, 0, 1);
        if (!layerBSDF->type().hasType(BSDFType::Transmission)) {
            if (glm::dot(norm, wGiven) < 0) {
                //norm = -norm;
            }
        }

        SurfaceIntr intr;
        intr.wo = Transform::worldToLocal(norm, wGiven);
        auto bsdfSample = layerBSDF->sample(intr, sampler->get3(), mode);

        if (!bsdfSample) {
            return std::nullopt;
        }
        bsdfSample->dir = Transform::localToWorld(norm, bsdfSample->dir);
        bSample = *bsdfSample;

        if (Math::isBlack(bSample.bsdf) || Math::isNan(pdf)) {
            return std::nullopt;
        }

        goingDown = bSample.dir.z < 0;
        bool isTop = layer == topLayer;

        throughput *= bSample.bsdf;
        //throughput *= bSample.bsdf / (bSample.type.isDelta() ? 1.f : glm::abs(bSample.dir.z));
        //throughput *= bSample.bsdf / glm::abs(bSample.dir.z);
        pdf *= bSample.pdf;

        wGiven = -bSample.dir;
        layer += goingDown ? 1 : -1;
    }
    if (bSample.type.isDelta()) {
        //throughput /= glm::abs(bSample.dir.z);
    }
    return BSDFSample(bSample.dir, throughput, pdf, BSDFType::Glossy | BSDFType::Reflection);
}

Spectrum LayeredBSDF2::bsdf(const SurfaceIntr& intr, TransportMode mode) const {
    auto wo = intr.wo;
    auto wi = intr.wi;
    if (wo.z < 0 || wi.z < 0) {
        return Spectrum(0.f);
    }

    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    Spectrum eval = top->bsdf(intr, mode);
    
    auto d1Sample = top->sample({ wi }, intr.sampler->get3(), ~mode);

    if (!d1Sample) {
        return eval;
    }
    if (d1Sample->dir.z > 0) {
        return eval;
    }

    if (!top->type().isDelta()) {
        auto d2Sample = bottom->sample({ -d1Sample->dir }, intr.sampler->get3(), ~mode);

        if (d2Sample) {
            float connectPdf = top->pdf({ wo, -d2Sample->dir }, mode);
            if (!Math::isNan(connectPdf) && connectPdf > 0) {
                float pdf = d1Sample->pdf * d2Sample->pdf;
                Spectrum thru = d1Sample->bsdf * d2Sample->bsdf * top->bsdf({ wo, -d2Sample->dir }, mode);
                float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(d2Sample->pdf, connectPdf);
                //weight = 1.f;
                eval += thru / pdf * weight;
            }
        }
    }
    auto d3Sample = top->sample({ wo }, intr.sampler->get3(), mode);

    if (d3Sample) {
        if (d3Sample->dir.z > 0) {
            return eval;
        }
        float pdf = d1Sample->pdf * d3Sample->pdf;
        Spectrum thru = d1Sample->bsdf * d3Sample->bsdf * bottom->bsdf({ -d3Sample->dir, -d1Sample->dir }, mode);
        float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(pdf, bottom->pdf({ -d3Sample->dir, -d1Sample->dir }, mode));
        //weight = 1.f;
        eval += thru / pdf * weight;
    }
    return eval;
}

float LayeredBSDF2::pdf(const SurfaceIntr& intr, TransportMode mode) const {
    auto wo = intr.wo;
    auto wi = intr.wi;
    if (wo.z < 0 || wi.z < 0) {
        return 0.f;
    }
    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    float eval = top->pdf(intr, mode);
    
    auto d1Sample = top->sample({ wi }, intr.sampler->get3(), ~mode);

    if (!d1Sample) {
        return eval;
    }
    if (d1Sample->dir.z > 0) {
        return eval;
    }

    if (!top->type().isDelta()) {
        auto d2Sample = bottom->sample({ -d1Sample->dir }, intr.sampler->get3(), ~mode);

        if (d2Sample) {
            float connectPdf = top->pdf({ wo, -d2Sample->dir }, mode);
            if (!Math::isNan(connectPdf) && connectPdf > 0) {
                float pdf = d1Sample->pdf * d2Sample->pdf;
                float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(d2Sample->pdf, connectPdf);
                //weight = 1.f;
                eval += pdf * weight;
            }
        }
    }
    auto d3Sample = top->sample({ wo }, intr.sampler->get3(), mode);

    if (d3Sample) {
        if (d3Sample->dir.z > 0) {
            return eval;
        }
        float pdf = d1Sample->pdf * d3Sample->pdf;
        float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(pdf, bottom->pdf({ -d3Sample->dir, -d1Sample->dir }, mode));
        //weight = 1.f;
        eval += pdf * weight;
    }
    return eval;
}

std::optional<BSDFSample> LayeredBSDF2::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) const {
    return generatePath(intr.wo, intr.uv, interfaces, normalMaps, intr.sampler, maxDepth, mode);
}

void LayeredBSDF2::addBSDF(BSDF* bsdf, Texture3fPtr normalMap) {
    interfaces.push_back(bsdf);
    normalMaps.push_back(normalMap);
    mType.type |= bsdf->type().type;
}