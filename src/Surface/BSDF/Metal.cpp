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


Spectrum MetalBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const {
    if (approxDelta()) {
        return Spectrum(0.f);
    }

    const auto &[wo, wi, uv, spTemp] = intr;
    Vec3f h = glm::normalize(wi + wo);
    float alpha = roughness * roughness;

    if (!Math::sameHemisphere(wo, wi)) {
        return Spectrum(0.0f);
    }
    float cosWo = glm::abs(wo.z);
    float cosWi = glm::abs(wi.z);

    if (cosWo * cosWi < 1e-7f) {
        return Spectrum(0.f);
    }

    Vec3f wh = glm::normalize(wo + wi);
    float fr = FresnelConductor(Math::absDot(wh, wo), eta, k);
    return baseColor.get(uv) * distrib.d(wh) * distrib.g(wo, wi) * fr / (4.f * cosWo * cosWi);
}

float MetalBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const {
    if (!Math::sameHemisphere(intr.wo, intr.wi) || approxDelta()) {
        return 0;
    }
    Vec3f wh = glm::normalize(intr.wo + intr.wi);
    return distrib.pdf(wh, intr.wo) / (4.0f * glm::dot(wh, intr.wo));
}

std::optional<BSDFSample> MetalBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) const {
    auto& wo = intr.wo;

    if (approxDelta()) {
        Vec3f wi(-wo.x, -wo.y, wo.z);
        float fr = FresnelConductor(glm::abs(wo.z), eta, k);
        return BSDFSample(wi, baseColor.get(intr.uv) * fr, 1.f, BSDFType::Delta | BSDFType::Reflection);
    }
    else {
        Vec3f wh = distrib.sampleWm(wo, { u.x, u.y });
        Vec3f wi = glm::reflect(-wo, wh);
        
        if (!Math::sameHemisphere(wo, wi)) {
            return std::nullopt;
        }
        SurfaceIntr newIntr(wo, wi, intr.uv);
        return BSDFSample(wi, bsdf(newIntr, mode), pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection);
    }
}