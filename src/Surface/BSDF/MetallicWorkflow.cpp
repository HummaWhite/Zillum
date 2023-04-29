#include "Core/BSDF.h"

Spectrum MetallicWorkflowBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    Vec3f wh = glm::normalize(wi + wo);
    float alpha = roughness * roughness;

    if (!Math::sameHemisphere(wo, wi)) {
        return Spectrum(0.0f);
    }
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);

    Spectrum base = baseColor.get(uv);
    Spectrum F0 = Math::lerp(Spectrum(0.04f), base, metallic);

    Spectrum f = SchlickF(Math::satDot(wh, wo), F0, roughness);
    float d = distrib.d(wh);
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

float MetallicWorkflowBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    Vec3f h = glm::normalize(wo + wi);

    float pdfDiff = wi.z * Math::PiInv;
    float pdfSpec = distrib.pdf(h, wo) / (4.0f * glm::dot(h, wo));
    return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
}

std::optional<BSDFSample> MetallicWorkflowBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    float spec = 1.0f / (2.0f - metallic);
    bool sampleDiff = sampler->get1() >= spec;
    
    Vec3f wi;
    if (sampleDiff) {
        wi = Math::sampleHemisphereCosine(sampler->get2());
    }
    else {
        auto wh = distrib.sampleWm(wo, sampler->get2());
        wi = glm::reflect(-wo, wh);
    }
    if (wi.z <= 0) {
        return std::nullopt;
    }
    return BSDFSample(wi, bsdf(wo, wi, uv, mode), pdf(wo, wi, uv, mode),
        (sampleDiff ? BSDFType::Diffuse : BSDFType::Glossy) | BSDFType::Reflection);
}