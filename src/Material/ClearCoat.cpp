#include "../../include/Core/Material.h"

Spectrum Clearcoat::bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    auto h = glm::normalize(wo + wi);

    float cosWo = Math::satDot(n, wo);
    float cosWi = Math::satDot(n, wi);

    float d = distrib.d(n, h);
    auto f = schlickF(Math::absDot(h, wo), Vec3f(0.04f));
    float g = smithG(n, wo, wi, 0.25f);

    float denom = 4.0f * cosWo * cosWi;
    if (denom < 1e-7f)
        return Spectrum(0.0f);

    return f * d * g * weight / denom;
}

float Clearcoat::pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    auto h = glm::normalize(wo + wi);
    return distrib.pdf(n, h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> Clearcoat::sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode)
{
    auto h = distrib.sampleWm(n, wo, { u.y, u.z });
    auto wi = glm::reflect(-wo, h);

    if (glm::dot(n, wi) < 0.0f)
        return std::nullopt;
    return BSDFSample(wi, pdf(n, wo, wi, mode), BXDF::GlosRefl, bsdf(n, wo, wi, mode));
}