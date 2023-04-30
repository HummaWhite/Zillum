#include "Core/BSDF.h"

float FresnelConductor(float cosI, float eta, float k) {
    Vec2c etak(eta, k);
    Vec2c cosThetaI(glm::clamp(cosI, 0.f, 1.f), 0.f);

    Vec2c sin2ThetaI(1.f - cosThetaI.lengthSqr(), 0.f);
    Vec2c sin2ThetaT = sin2ThetaI / (etak * etak);
    Vec2c cosThetaT = (Vec2c(1.f, 0.f) - sin2ThetaT).sqrt();

    Vec2c rPa = (etak * cosThetaI - cosThetaT) / (etak * cosThetaI + cosThetaT);
    Vec2c rPe = (cosThetaI - etak * cosThetaT) / (cosThetaI + etak * cosThetaT);
    return (rPa.lengthSqr() + rPe.lengthSqr()) * .5f;
}

Spectrum MetalBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    if (!Math::sameHemisphere(wo, wi)) {
        return Spectrum(0.0f);
    }
    Vec3f wh = glm::normalize(wi + wo);
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);

    if (cosWo * cosWi < 1e-7f) {
        return Spectrum(0.f);
    }
    return baseColor.get(uv) * FresnelConductor(Math::absDot(wh, wo), eta, k) *
        distrib.d(wh) * distrib.g(wo, wi) / (4.f * cosWi * cosWo);
}

float MetalBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    Vec3f wh = glm::normalize(wo + wi);
    return distrib.pdf(wh, wo) / (4.0f * Math::absDot(wh, wo));
}

std::optional<BSDFSample> MetalBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler, BSDFType component) const {
    if (approxDelta()) {
        Vec3f wi(-wo.x, -wo.y, wo.z);
        float fr = FresnelConductor(glm::abs(wo.z), eta, k);
        return BSDFSample(wi, baseColor.get(uv) * fr, 1.f, BSDFType::Delta | BSDFType::Reflection);
    }
    else {
        Vec3f wh = distrib.sampleWm(wo, sampler->get2());
        Vec3f wi = glm::reflect(-wo, wh);
        
        if (!Math::sameHemisphere(wo, wi)) {
            return std::nullopt;
        }
        return BSDFSample(wi, bsdf(wo, wi, uv, mode), pdf(wo, wi, uv, mode), BSDFType::Glossy | BSDFType::Reflection);
    }
}