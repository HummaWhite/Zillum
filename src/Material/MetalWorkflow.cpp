#include "Material.h"

Vec3f MetalWorkflow::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    Vec3f H = glm::normalize(Wi + Wo);
    float alpha = roughness * roughness;

    if (dot(N, Wi) < 1e-10f)
        return Vec3f(0.0f);
    if (dot(N, Wo) < 1e-10f)
        return Vec3f(0.0f);

    float NoL = Math::satDot(N, Wi);
    float NoV = Math::satDot(N, Wo);

    Vec3f F0 = glm::mix(Vec3f(0.04f), albedo, metallic);

    Vec3f F = Microfacet::schlickF(Math::satDot(H, Wo), F0, roughness);
    float D = distrib.d(N, H);
    float G = distrib.g(N, Wo, Wi);

    Vec3f ks = F;
    Vec3f kd = Vec3f(1.0f) - ks;
    kd *= 1.0f - metallic;

    Vec3f FDG = F * D * G;
    float denom = 4.0f * NoV * NoL;
    if (denom < 1e-7f)
        return Vec3f(0.0f);

    Vec3f glossy = FDG / denom;

    return kd * albedo * Math::PiInv + glossy;
}

float MetalWorkflow::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    float NoWi = glm::dot(N, Wi);
    Vec3f H = glm::normalize(Wo + Wi);

    float pdfDiff = NoWi * Math::PiInv;
    float pdfSpec = distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
    return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
}

Sample MetalWorkflow::getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    float spec = 1.0f / (2.0f - metallic);
    bool sampleDiff = u1 > spec;

    Vec3f Wi;
    if (sampleDiff)
        Wi = Math::sampleHemisphereCosine(N, u2).first;
    else
    {
        auto H = distrib.sampleWm(N, Wo, u2);
        Wi = glm::reflect(-Wo, H);
    }

    float NoWi = glm::dot(N, Wi);
    if (NoWi < 0.0f)
        return Sample();

    return Sample(Wi, pdf(N, Wo, Wi, mode), sampleDiff ? BXDF::Diffuse : BXDF::GlosRefl);
}