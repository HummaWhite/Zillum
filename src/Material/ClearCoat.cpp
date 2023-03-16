#include "Core/BSDF.h"

Spectrum Clearcoat::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
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

float Clearcoat::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    auto h = glm::normalize(wo + wi);
    return distrib.pdf(n, h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> Clearcoat::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    auto h = distrib.sampleWm(intr.n, intr.wo, { u.y, u.z });
    auto wi = glm::reflect(-intr.wo, h);

    if (glm::dot(intr.n, wi) < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection, bsdf(newIntr, mode));
}