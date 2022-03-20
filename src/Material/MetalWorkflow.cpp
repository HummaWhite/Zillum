#include "../../include/Core/Material.h"

Spectrum MetalWorkflow::bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    Vec3f h = glm::normalize(wi + wo);
    float alpha = roughness * roughness;

    if (!Math::sameHemisphere(n, wo, wi))
        return Spectrum(0.0f);

    float NoL = Math::satDot(n, wi);
    float NoV = Math::satDot(n, wo);

    Spectrum F0 = Math::lerp(Spectrum(0.04f), albedo, metallic);

    Spectrum f = schlickF(Math::satDot(h, wo), F0, roughness);
    float d = distrib.d(n, h);
    float g = distrib.g(n, wo, wi);

    Spectrum ks = f;
    Spectrum kd = Vec3f(1.0f) - ks;
    kd *= 1.0f - metallic;

    float denom = 4.0f * NoV * NoL;
    if (denom < 1e-7f)
        return Vec3f(0.0f);
    Vec3f glossy = f * d * g / denom;

    return kd * albedo * Math::PiInv + glossy;
}

float MetalWorkflow::pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    float NoWi = glm::dot(n, wi);
    Vec3f h = glm::normalize(wo + wi);

    float pdfDiff = NoWi * Math::PiInv;
    float pdfSpec = distrib.pdf(n, h, wo) / (4.0f * glm::dot(h, wo));
    return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
}

std::optional<BSDFSample> MetalWorkflow::sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode)
{
    float spec = 1.0f / (2.0f - metallic);
    bool sampleDiff = u.x >= spec;
    Vec2f u2(u.y, u.z);
    
    Vec3f wi;
    if (sampleDiff)
        wi = Math::sampleHemisphereCosine(n, u2).first;
    else
    {
        auto h = distrib.sampleWm(n, wo, u2);
        wi = glm::reflect(-wo, h);
    }
    if (glm::dot(n, wi) <= 0)
        return std::nullopt;
    return BSDFSample(wi, pdf(n, wo, wi, mode), sampleDiff ? BXDF::Diffuse : BXDF::GlosRefl, bsdf(n, wo, wi, mode));
}