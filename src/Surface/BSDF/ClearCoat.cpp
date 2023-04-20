#include "Core/BSDF.h"

Spectrum ClearcoatBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const
{
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

float ClearcoatBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const
{
    auto wh = glm::normalize(wo + wi);
    return distrib.pdf(wh, wo) / (4.0f * glm::dot(wh, wo));
}

std::optional<BSDFSample> ClearcoatBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler) const
{
    auto wh = distrib.sampleWm(wo, sampler->get2());
    auto wi = glm::reflect(-wo, wh);

    if (wi.z < 0.0f)
        return std::nullopt;
    return BSDFSample(wi, bsdf(wo, wi, uv, mode), pdf(wo, wi, uv, mode), BSDFType::Glossy | BSDFType::Reflection);
}