#include "../../include/Core/Material.h"

Spectrum Clearcoat::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);

    float NoWo = Math::satDot(N, Wo);
    float NoWi = Math::satDot(N, Wi);

    float D = distrib.d(N, H);
    auto F = schlickF(Math::absDot(H, Wo), Vec3f(0.04f));
    float G = smithG(N, Wo, Wi, 0.25f);

    float denom = 4.0f * NoWo * NoWi;
    if (denom < 1e-7f)
        return Spectrum(0.0f);

    return F * D * G * weight / denom;
}

float Clearcoat::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);
    return distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
}

std::optional<BSDFSample> Clearcoat::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto H = distrib.sampleWm(N, Wo, u2);
    auto Wi = glm::reflect(-Wo, H);

    if (glm::dot(N, Wi) < 0.0f)
        return std::nullopt;
    return BSDFSample(Wi, pdf(N, Wo, Wi, mode), BXDF::GlosRefl, bsdf(N, Wo, Wi, mode));
}