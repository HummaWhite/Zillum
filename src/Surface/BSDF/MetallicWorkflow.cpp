#include "Core/BSDF.h"

Spectrum MetallicWorkflowBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const {
    const auto &[wo, wi, uv, spTemp] = intr;
    Vec3f h = glm::normalize(wi + wo);
    float alpha = roughness * roughness;

    if (!Math::sameHemisphere(wo, wi)) {
        return Spectrum(0.0f);
    }
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);

    Spectrum base = baseColor.get(intr.uv);
    Spectrum F0 = Math::lerp(Spectrum(0.04f), base, metallic);

    Spectrum f = schlickF(Math::satDot(h, wo), F0, roughness);
    float d = distrib.d(h);
    float g = distrib.g(wo, wi);

    Spectrum ks = f;
    Spectrum kd = Vec3f(1.0f) - ks;
    kd *= 1.0f - metallic;

    float denom = 4.0f * cosWo * cosWi;
    if (denom < 1e-7f) {
        return Vec3f(0.0f);
    }
    Vec3f glossy = f * d * g / denom;

    return kd * base * Math::PiInv + glossy;
}

float MetallicWorkflowBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const {
    const auto &[wo, wi, uv, spTemp] = intr;
    Vec3f h = glm::normalize(wo + wi);

    float pdfDiff = wi.z * Math::PiInv;
    float pdfSpec = distrib.pdf(h, wo) / (4.0f * glm::dot(h, wo));
    return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
}

std::optional<BSDFSample> MetallicWorkflowBSDF::sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) const {
    auto &wo = intr.wo;
    float spec = 1.0f / (2.0f - metallic);
    bool sampleDiff = u.x >= spec;
    Vec2f u2(u.y, u.z);
    
    Vec3f wi;
    if (sampleDiff) {
        wi = Math::sampleHemisphereCosine({ u.y, u.z });
    }
    else {
        auto h = distrib.sampleWm(wo, { u.y, u.z });
        wi = glm::reflect(-wo, h);
    }
    if (wi.z <= 0) {
        return std::nullopt;
    }
    SurfaceIntr newIntr(wo, wi, intr.uv);
    return BSDFSample(wi, bsdf(newIntr, mode), pdf(newIntr, mode), (sampleDiff ? BSDFType::Diffuse : BSDFType::Glossy) | BSDFType::Reflection);
}