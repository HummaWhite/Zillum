#include "Core/BSDF.h"
#include "Utils/Error.h"

struct PathVertex {
    int layer;
    Vec3f wGiven, wSampled;
    Spectrum fRegular, fAdjoint;
    float pdfRegular, pdfAdjoint;
};

/*
float pdfTRT(const SurfaceIntr& intr, const std::vector<BSDF*>& BSDFs, TransportMode mode) {
    int nBSDFs = BSDFs.size();
    SurfaceIntr lintr = intr.swapInOut();
    Sampler* sampler = intr.sampler;

    std::vector<Vec3f> wo(nBSDFs);
    std::vector<Vec3f> wi(nBSDFs);
    std::vector<float> ratio(nBSDFs);

    auto getWoId = [&](int layer) {
        return (intr.cosWo() > 0) ? layer : nBSDFs - layer - 1;
    };

    auto getWiId = [&](int layer) {
        return (intr.cosWi() > 0) ? layer : nBSDFs - layer - 1;
    };

    wo[0] = intr.wo;
    wi[0] = intr.wi;
    ratio[0] = 1.f;
    float pdf = 0.f;

    for (int i = 0; i < nBSDFs - 1; i++) {
        int wiId = getWiId(i);
        lintr.wi = wi[i];
        auto woSample = BSDFs[wiId]->sample(lintr.swapInOut(), sampler->get3(), mode);
        if (!woSample) {
            return 0.f;
        }
        wo[i + 1] = -woSample->dir;

        int woId = getWoId(i);
        lintr.wo = wo[i];
        auto wiSample = BSDFs[woId]->sample(lintr, sampler->get3(), mode);
        if (!wiSample) {
            return 0.f;
        }
        lintr.wi = wi[i + 1] = -wiSample->dir;

        bool isDelta = BSDFs[wiId]->type().isDelta();
        float pdf0 = BSDFs[wiId]->pdf(lintr, mode);
        float pdf1 = BSDFs[wiId]->pdf(lintr.swapInOut(), mode);

        if (pdf0 > 0) {
            ratio[i + 1] = ratio[i] * pdf1 / pdf0;
        }
        lintr = lintr.swapInOut();
    }

    for (int i = 1; i < nBSDFs; i++) {
        if (wi[i].z > 0 && wo[i].z > 0) {
            pdf += BSDFs[getWiId(i)]->pdf({ lintr.n, wo[i], wi[i] }, mode) * ratio[i];
        }
    }
    return pdf + .1f;
    //return pdf;
}
*/

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
        Vec3f norm = normalMaps[layer] ? (normalMaps[layer]->get(uv.x, uv.y) * 2.f - 1.f) : LocalUp;
        if (!layerBSDF->type().hasType(BSDFType::Transmission)) {
            if (glm::dot(norm, wGiven) < 0) {
                //norm = -norm;
            }
        }

        SurfaceIntr intr;
        intr.n = norm;
        intr.wo = wGiven;
        auto bsdfSample = layerBSDF->sample(intr, sampler->get3(), mode);

        if (!bsdfSample) {
            return std::nullopt;
        }
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

Spectrum LayeredBSDF::bsdf(const SurfaceIntr& intr, TransportMode mode) {
    auto [wo, wi] = intr.localDirections();
    if (wo.z < 0 || wi.z < 0) {
        return Spectrum(0.f);
    }

    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    Spectrum eval = top->bsdf(intr.localized(), mode);
    
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

float LayeredBSDF::pdf(const SurfaceIntr& intr, TransportMode mode) {
    auto [wo, wi] = intr.localDirections();
    if (wo.z < 0 || wi.z < 0) {
        return 0.f;
    }
    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    float eval = top->pdf(intr.localized(), mode);
    
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

std::optional<BSDFSample> LayeredBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) {
    auto localIntr = intr.localized();
    auto bSample = generatePath(localIntr.wo, intr.uv, interfaces, normalMaps, intr.sampler, maxDepth, mode);
    if (!bSample) {
        return std::nullopt;
    }
    localIntr.wi = bSample->dir;

    Spectrum bsdf = bSample->bsdf;
    float pdf = bSample->pdf;
    //bsdf = this->bsdf(localIntr, mode);
    //pdf = this->pdf(localIntr, mode);
    return BSDFSample(Transform::localToWorld(intr.n, bSample->dir), bsdf, pdf, bSample->type);
}

void LayeredBSDF::addBSDF(BSDF* bsdf, Texture3fPtr normalMap) {
    interfaces.push_back(bsdf);
    normalMaps.push_back(normalMap);
    mType.type |= bsdf->type().type;
}