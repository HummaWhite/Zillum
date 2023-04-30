#include "Core/BSDF.h"
#include "Core/PhaseFunction.h"
#include "Utils/Error.h"

float transmittance(float z0, float z1, Vec3f w) {
    return glm::exp(-glm::abs((z0 - z1) / w.z));
}

float transmittance(float dz, Vec3f w) {
    return glm::exp(-glm::abs(dz / w.z));
}

float sampleExponential(float a, float u) {
    return -glm::log(1 - u) / a;
}

Spectrum LayeredBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    if (!top && !bottom) {
        return Spectrum(0.f);
    }
    else if (!top) {
        return bottom->bsdf(wo, wi, uv, mode, params);
    }
    else if (!bottom) {
        return top->bsdf(wo, wi, uv, mode, params);
    }

    if (twoSided && wo.z < 0) {
        wo = -wo;
        wi = -wi;
    }
    bool entTop = twoSided || wo.z > 0;
    BSDF* ent = entTop ? top : bottom;
    BSDF* oth = entTop ? bottom : top;
    BSDF* ext = Math::sameHemisphere(wo, wi) ? ent : oth;

    float zEnt = entTop ? 0 : thickness;
    float zExt = (ext == ent) ? zEnt : thickness - zEnt;

    Spectrum f(0.f);
    Spectrum alb = albedo.get(uv);

    if (Math::sameHemisphere(wo, wi)) {
        f += ent->bsdf(wo, wi, uv, mode, params) * static_cast<float>(nSamples);
    }

    for (int i = 0; i < nSamples; i++) {
        auto wos = ent->sample(wo, uv, mode, params.sampler, BSDFType::Transmission);

        if (!wos || Math::isBlack(wos->bsdf) || wos->pdf < 1e-8f || wos->w.z == 0) {
            continue;
        }
        auto wis = ext->sample(wi, uv, ~mode, params.sampler, BSDFType::Transmission);
        
        if (!wis || Math::isBlack(wis->bsdf) || wis->pdf < 1e-8f || wis->w.z == 0) {
            continue;
        }

        Spectrum throughput = wos->bsdf / wos->pdf * (wos->type.isDelta() ? 1.f : glm::abs(wos->w.z));
        float z = entTop ? 0 : thickness;
        Vec3f w = wos->w;

        for (int depth = 1; depth <= maxDepth; depth++) {
            if (depth > 4 && Math::luminance(throughput) < .25f) {
                float rr = glm::max(0.f, 1.f - Math::luminance(throughput));
                if (params.sampler->get1() < rr) {
                    break;
                }
                throughput /= (1.f - rr);
            }

            if (Math::isBlack(alb)) {
                z = (z == thickness) ? 0 : thickness;
                throughput *= transmittance(thickness, w);
            }
            else {
                float sigT = 1.f;
                float dz = sampleExponential(sigT / glm::abs(w.z), params.sampler->get1());

                if (dz == 0) {
                    continue;
                }
                float zNext = (w.z > 0) ? z - dz : z + dz;

                if (zNext < thickness && zNext > 0) {
                    float weight = 1.f;
                    if (!ext->type().isDelta()) {
                        weight = Math::powerHeuristic(wis->pdf, HGPhasePDF(-w, -wis->w, g));
                    }
                    f += wis->bsdf / wis->pdf * transmittance(zNext, zExt, wis->w) * alb *
                        HGPhaseFunction(glm::dot(-w, -wis->w), g) * weight * throughput;

                    auto phaseSample = HGPhaseSample(-w, g, params.sampler->get2());

                    if (phaseSample.pdf == 0 || phaseSample.w.z == 0) {
                        continue;
                    }
                    throughput *= alb * phaseSample.p / phaseSample.pdf;
                    w = phaseSample.w;
                    z = zNext;

                    if (((z > zExt && w.z > 0) || (z < zExt && w.z < 0)) && !ext->type().isDelta()) {
                        Spectrum fExt = ext->bsdf(-w, wi, uv, mode, params.sampler);
                        if (!Math::isBlack(fExt)) {
                            float pExt = ext->pdf(-w, wi, uv, mode, { params.sampler, BSDFType::Transmission });
                            float weight = Math::powerHeuristic(phaseSample.pdf, pExt);
                            f += fExt * transmittance(zNext, zExt, phaseSample.w) * weight * throughput;
                        }
                    }
                    continue;
                }
                z = glm::clamp(zNext, 0.f, thickness);
            }

            if (z == zExt) {
                auto es = ext->sample(-w, uv, mode, params.sampler, BSDFType::Reflection);
                if (!es || Math::isBlack(es->bsdf) || es->pdf < 1e-8f || es->w.z == 0) {
                    break;
                }
                throughput *= es->bsdf / es->pdf * (es->type.isDelta() ? 1.f : glm::abs(es->w.z));
                w = es->w;
            }
            else {
                if (!oth->type().isDelta()) {
                    float weight = 1.f;
                    if (!ext->type().isDelta()) {
                        weight = Math::powerHeuristic(wis->pdf, oth->pdf(-w, -wis->w, uv, mode, params.sampler));
                    }
                    f += oth->bsdf(-w, -wis->w, uv, mode, params.sampler) * glm::abs(wis->w.z) *
                        transmittance(thickness, wis->w) * wis->bsdf / wis->pdf * weight * throughput;
                }

                auto os = oth->sample(-w, uv, mode, params.sampler, BSDFType::Reflection);
                if (!os || Math::isBlack(os->bsdf) || os->pdf < 1e-8f || os->w.z == 0) {
                    break;
                }
                throughput *= os->bsdf / os->pdf * (os->type.isDelta() ? 1.f : glm::abs(os->w.z));
                w = os->w;

                if (!ext->type().isDelta()) {
                    Spectrum fExt = ext->bsdf(-w, wi, uv, mode, params.sampler);
                    if (!Math::isBlack(fExt)) {
                        float weight = 1.f;
                        if (!oth->type().isDelta()) {
                            float pExt = ext->pdf(-w, wi, uv, mode, { params.sampler, BSDFType::Transmission });
                            weight = Math::powerHeuristic(os->pdf, pExt);
                        }
                        f += fExt * transmittance(thickness, os->w) * weight * throughput;
                    }
                }
            }
        }
    }
    return f / static_cast<float>(nSamples);
}

float LayeredBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    if (!top && !bottom) {
        return 0;
    }
    else if (!top) {
        return bottom->pdf(wo, wi, uv, mode, params.sampler);
    }
    else if (!bottom) {
        return top->pdf(wo, wi, uv, mode, params.sampler);
    }

    if (twoSided && wo.z < 0) {
        wo = -wo;
        wi = -wi;
    }

    bool entTop = twoSided || wo.z > 0;
    float pdfSum = 0.f;

    if (Math::sameHemisphere(wo, wi)) {
        pdfSum += (entTop ? top->pdf(wo, wi, uv, mode, { params.sampler, BSDFType::Reflection }) :
            bottom->pdf(wo, wi, uv, mode, { params.sampler, BSDFType::Reflection })) * nSamples;
    }

    for (int i = 0; i < nSamples; i++) {
        if (Math::sameHemisphere(wo, wi)) {
            BSDF* tBSDF = entTop ? top : bottom;
            BSDF* rBSDF = entTop ? bottom : top;

            auto wos = tBSDF->sample(wo, uv, mode, params.sampler, BSDFType::Transmission);
            auto wis = tBSDF->sample(wi, uv, ~mode, params.sampler, BSDFType::Transmission);

            if (wos && !Math::isBlack(wos->bsdf) && wos->pdf > 1e-8f &&
                wis && !Math::isBlack(wis->bsdf) && wis->pdf > 1e-8f) {
                if (tBSDF->type().isDelta()) {
                    pdfSum += rBSDF->pdf(-wos->w, -wis->w, uv, mode, params.sampler);
                }
                else {
                    auto rs = rBSDF->sample(-wos->w, uv, mode, params.sampler);
                    if (rs && !Math::isBlack(rs->bsdf) && rs->pdf > 1e-8f) {
                        if (rBSDF->type().isDelta()) {
                            pdfSum += tBSDF->pdf(-rs->w, wi, uv, mode, params.sampler);
                        }
                        else {
                            float rPdf = rBSDF->pdf(-wos->w, -wis->w, uv, mode, params.sampler);
                            pdfSum += rPdf * Math::powerHeuristic(wis->pdf, rPdf);

                            float tPdf = tBSDF->pdf(-rs->w, wi, uv, mode, params.sampler);
                            pdfSum += tPdf * Math::powerHeuristic(rs->pdf, tPdf);
                        }
                    }
                }
            }
        }
        else {
            BSDF* oBSDF = entTop ? top : bottom;
            BSDF* iBSDF = entTop ? bottom : top;

            auto wos = oBSDF->sample(wo, uv, mode, params.sampler);

            if (!wos || Math::isBlack(wos->bsdf) || wos->pdf < 1e-8f || wos->w.z == 0 ||
                !wos->type.isTransmission()) {
                continue;
            }
            auto wis = iBSDF->sample(wi, uv, ~mode, params.sampler);

            if (!wis || Math::isBlack(wis->bsdf) || wis->pdf < 1e-8f || wis->w.z == 0 ||
                !wis->type.isTransmission()) {
                continue;
            }

            if (oBSDF->type().isDelta()) {
                pdfSum += iBSDF->pdf(-wos->w, wi, uv, mode, params.sampler);
            }
            else if (iBSDF->type().isDelta()) {
                pdfSum += oBSDF->pdf(wo, -wis->w, uv, mode, params.sampler);
            }
            else {
                pdfSum += iBSDF->pdf(-wos->w, wi, uv, mode, params.sampler) * .5f +
                    oBSDF->pdf(wo, -wis->w, uv, mode, params.sampler) * .5f;
            }
        }
    }
    return glm::mix(.25f * Math::PiInv, pdfSum / nSamples, .9f);
}

std::optional<BSDFSample> LayeredBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler, BSDFType component) const {
    if (!top && !bottom) {
        return std::nullopt;
    }
    else if (!top) {
        return bottom->sample(wo, uv, mode, sampler);
    }
    else if (!bottom) {
        return top->sample(wo, uv, mode, sampler);
    }
    
    bool entTop = wo.z > 0;
    BSDF* ent = entTop ? top : bottom;
    BSDF* oth = entTop ? bottom : top;

    Texture3fPtr entNormMap = entTop ? topNormal : bottomNormal;

    auto ins = !entNormMap ? ent->sample(wo, uv, mode, sampler) :
        ent->sample(entNormMap->getNormal(uv), wo, uv, mode, sampler);

    if (!ins || ins->pdf < 1e-8f || ins->w.z == 0 || Math::isBlack(ins->bsdf)) {
        return std::nullopt;
    }

    if (Math::sameHemisphere(wo, ins->w)) {
        return ins;
    }

    Spectrum f = ins->bsdf * (ins->type.isDelta() ? 1.f : glm::abs(ins->w.z));
    float pdf = ins->pdf;
    float z = entTop ? 0.f : thickness;
    Vec3f w = ins->w;
    Spectrum alb = albedo.get(uv);
    bool delta = ins->type.isDelta();

    for (int depth = 1; depth <= maxDepth; depth++) {
        if (depth > 4) {
            float rr = glm::max(0.f, 1.f - Math::luminance(f) / pdf);
            if (sampler->get1() < rr) {
                return std::nullopt;
            }
            pdf *= 1.f - rr;
        }

        if (w.z == 0) {
            return std::nullopt;
        }

        if (Math::isBlack(alb)) {
            z = (z == thickness) ? 0 : thickness;
            f *= transmittance(thickness, w);
        }
        else {
            float sigT = 1.f;
            float dz = sampleExponential(sigT / glm::abs(w.z), sampler->get1());

            if (dz == 0) {
                continue;
            }
            float zNext = (w.z > 0) ? z - dz : z + dz;

            if (zNext < thickness && zNext > 0) {
                auto phaseSample = HGPhaseSample(-w, g, sampler->get2());

                if (phaseSample.pdf == 0 || phaseSample.w.z == 0) {
                    return std::nullopt;
                }
                f *= alb * phaseSample.p;
                pdf *= phaseSample.pdf;
                w = phaseSample.w;
                z = zNext;
                delta = false;
                continue;
            }
            z = glm::clamp(zNext, 0.f, thickness);
        }

        BSDF* interf = (z == 0) ? top : bottom;
        Texture3fPtr interfNormMap = (z == 0) ? topNormal : bottomNormal;
        auto bsdfSample = !interfNormMap ? interf->sample(-w, uv, mode, sampler) :
            interf->sample(interfNormMap->getNormal(uv), -w, uv, mode, sampler);

        if (!bsdfSample || Math::isBlack(bsdfSample->bsdf) || bsdfSample->pdf == 0 ||
            bsdfSample->w.z == 0 || Math::isNan(bsdfSample->pdf)) {
            return std::nullopt;
        }
        f *= bsdfSample->bsdf;
        pdf *= bsdfSample->pdf;
        delta &= bsdfSample->type.isDelta();
        w = bsdfSample->w;

        if (bsdfSample->type.isTransmission()) {
            int roughness = delta ? BSDFType::Delta : BSDFType::Glossy;
            int direction = Math::sameHemisphere(wo, w) ? BSDFType::Reflection : BSDFType::Transmission;
            if (Math::hasNan(f) || Math::isNan(pdf) || pdf < 1e-8f) {
                return std::nullopt;
            }
            return BSDFSample(w, f, pdf, roughness | direction);
        }
        if (!bsdfSample->type.isDelta()) {
            f *= glm::abs(bsdfSample->w.z);
        }
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

        auto bsdfSample = layerBSDF->sample(Transform::worldToLocal(norm, wGiven), uv, mode, sampler);

        if (!bsdfSample) {
            return std::nullopt;
        }
        bsdfSample->w = Transform::localToWorld(norm, bsdfSample->w);
        bSample = *bsdfSample;

        if (Math::isBlack(bSample.bsdf) || Math::isNan(pdf)) {
            return std::nullopt;
        }

        goingDown = bSample.w.z < 0;
        bool isTop = layer == topLayer;

        throughput *= bSample.bsdf;
        //throughput *= bSample.bsdf / (bSample.type.isDelta() ? 1.f : glm::abs(bSample.dir.z));
        //throughput *= bSample.bsdf / glm::abs(bSample.dir.z);
        pdf *= bSample.pdf;

        wGiven = -bSample.w;
        layer += goingDown ? 1 : -1;
    }
    if (bSample.type.isDelta()) {
        //throughput /= glm::abs(bSample.dir.z);
    }
    return BSDFSample(bSample.w, throughput, pdf, BSDFType::Glossy | BSDFType::Reflection);
}

Spectrum LayeredBSDF2::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    if (wo.z < 0 || wi.z < 0) {
        return Spectrum(0.f);
    }

    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    Spectrum eval = top->bsdf(wo, wi, uv, mode, params.sampler);
    
    auto d1Sample = top->sample(wi, uv, ~mode, params.sampler);

    if (!d1Sample) {
        return eval;
    }
    if (d1Sample->w.z > 0) {
        return eval;
    }

    if (!top->type().isDelta()) {
        auto d2Sample = bottom->sample(-d1Sample->w, uv, ~mode, params.sampler);

        if (d2Sample) {
            float connectPdf = top->pdf(wo, -d2Sample->w, uv, mode, params.sampler);
            if (!Math::isNan(connectPdf) && connectPdf > 0) {
                float pdf = d1Sample->pdf * d2Sample->pdf;
                Spectrum thru = d1Sample->bsdf * d2Sample->bsdf * top->bsdf(wo, -d2Sample->w, uv, mode, params.sampler);
                float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(d2Sample->pdf, connectPdf);
                //weight = 1.f;
                eval += thru / pdf * weight;
            }
        }
    }
    auto d3Sample = top->sample(wo, uv, mode, params.sampler);

    if (d3Sample) {
        if (d3Sample->w.z > 0) {
            return eval;
        }
        float pdf = d1Sample->pdf * d3Sample->pdf;
        Spectrum thru = d1Sample->bsdf * d3Sample->bsdf * bottom->bsdf(-d3Sample->w, -d1Sample->w, uv, mode, params.sampler);
        float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(pdf, bottom->pdf(-d3Sample->w, -d1Sample->w, uv, mode, params.sampler));
        //weight = 1.f;
        eval += thru / pdf * weight;
    }
    return eval;
}

float LayeredBSDF2::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    if (wo.z < 0 || wi.z < 0) {
        return 0.f;
    }
    BSDF* top = interfaces[0];
    BSDF* bottom = interfaces[1];
    float eval = top->pdf(wo, wi, uv, mode, params.sampler);
    
    auto d1Sample = top->sample(wi, uv, ~mode, params.sampler);

    if (!d1Sample) {
        return eval;
    }
    if (d1Sample->w.z > 0) {
        return eval;
    }

    if (!top->type().isDelta()) {
        auto d2Sample = bottom->sample(-d1Sample->w, uv, ~mode, params.sampler);

        if (d2Sample) {
            float connectPdf = top->pdf(wo, -d2Sample->w, uv, mode);
            if (!Math::isNan(connectPdf) && connectPdf > 0) {
                float pdf = d1Sample->pdf * d2Sample->pdf;
                float weight = top->type().isDelta() ? 1.f : Math::powerHeuristic(d2Sample->pdf, connectPdf);
                //weight = 1.f;
                eval += pdf * weight;
            }
        }
    }
    auto d3Sample = top->sample(wo, uv, mode, params.sampler);

    if (d3Sample) {
        if (d3Sample->w.z > 0) {
            return eval;
        }
        float pdf = d1Sample->pdf * d3Sample->pdf;
        float weight = top->type().isDelta() ? 1.f :
            Math::powerHeuristic(pdf, bottom->pdf(-d3Sample->w, -d1Sample->w, uv, mode, params.sampler));
        //weight = 1.f;
        eval += pdf * weight;
    }
    return eval;
}

std::optional<BSDFSample> LayeredBSDF2::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler, BSDFType component) const {
    return generatePath(wo, uv, interfaces, normalMaps, sampler, maxDepth, mode);
}

void LayeredBSDF2::addBSDF(BSDF* bsdf, Texture3fPtr normalMap) {
    interfaces.push_back(bsdf);
    normalMaps.push_back(normalMap);
    mType.type |= bsdf->type().type;
}