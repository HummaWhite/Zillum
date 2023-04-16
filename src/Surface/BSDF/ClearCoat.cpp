#include "Core/BSDF.h"

Spectrum ClearcoatBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    auto wh = glm::normalize(wo + wi);

    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);

    float d = distrib.d(wh);
    auto f = schlickF(Math::absDot(wh, wo), Vec3f(0.04f));
    float g = smithG(wo, wi, 0.25f);

    float denom = 4.0f * cosWo * cosWi;
    if (denom < 1e-7f)
        return Spectrum(0.0f);

    return f * d * g * weight / denom;
}

float ClearcoatBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    auto wh = glm::normalize(wo + wi);
    return distrib.pdf(wh, wo) / (4.0f * glm::dot(wh, wo));
}

std::optional<BSDFSample> ClearcoatBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    auto h = distrib.sampleWm(intr.wo, { u.y, u.z });
    auto wi = glm::reflect(-intr.wo, h);

    if (wi.z < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, bsdf(newIntr, mode), pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection);
}