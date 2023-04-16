#include "Core/BSDF.h"

bool refract(Vec3f& wt, const Vec3f& wi, float eta)
{
    float cosTi = wi.z;
    if (cosTi < 0)
        eta = 1.0f / eta;
    float sin2Ti = glm::max<float>(0.0f, 1.0f - cosTi * cosTi);
    float sin2Tt = sin2Ti / (eta * eta);

    if (sin2Tt >= 1.0f)
        return false;

    float cosTt = glm::sqrt(1.0f - sin2Tt);
    if (cosTi < 0)
        cosTt = -cosTt;
    wt = glm::normalize(-wi / eta + Vec3f(0, 0, 1) * (cosTi / eta - cosTt));
    return true;
}

bool refract(Vec3f& wt, const Vec3f& wi, const Vec3f& n, float eta)
{
    float cosTi = glm::dot(n, wi);
    if (cosTi < 0)
        eta = 1.0f / eta;
    float sin2Ti = glm::max<float>(0.0f, 1.0f - cosTi * cosTi);
    float sin2Tt = sin2Ti / (eta * eta);

    if (sin2Tt >= 1.0f)
        return false;

    float cosTt = glm::sqrt(1.0f - sin2Tt);
    if (cosTi < 0)
        cosTt = -cosTt;
    wt = glm::normalize(-wi / eta + n * (cosTi / eta - cosTt));
    return true;
}

float fresnelDielectric(float cosTi, float eta)
{
    cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
    if (cosTi < 0.0f)
    {
        eta = 1.0f / eta;
        cosTi = -cosTi;
    }

    float sinTi = glm::sqrt(1.0f - cosTi * cosTi);
    float sinTt = sinTi / eta;
    if (sinTt >= 1.0f)
        return 1.0f;

    float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

    float rPa = (cosTi - eta * cosTt) / (cosTi + eta * cosTt);
    float rPe = (eta * cosTi - cosTt) / (eta * cosTi + cosTt);
    return (rPa * rPa + rPe * rPe) * 0.5f;
}

Spectrum DielectricBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    if (approxDelta)
        return Spectrum(0.0f);

    const auto &[wo, wi, uv, spTemp] = intr;
    auto wh = glm::normalize(wo + wi);
    float whCosWo = Math::absDot(wh, wo);
    float whCosWi = Math::absDot(wh, wi);

    if (Math::sameHemisphere(wo, wi))
    {
        float fr = fresnelDielectric(Math::absDot(wh, wi), ior);
        return (whCosWo * whCosWi < 1e-7f) ? Spectrum(0.0f) : baseColor * distrib.d(wh) * distrib.g(wo, wi) / (4.0f * whCosWo * whCosWi) * fr;
    }
    else
    {
        float eta = wi.z > 0 ? ior : 1.0f / ior;
        float sqrtDenom = glm::dot(wh, wo) + eta * glm::dot(wh, wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = whCosWi / denom;

        denom *= glm::abs(wi.z * wo.z);
        float fr = fresnelDielectric(glm::dot(wh, wi), eta);
        float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

        return (denom < 1e-7f) ?
            Spectrum(0.0f) :
            baseColor * glm::abs(distrib.d(wh) * distrib.g(wo, wi) * whCosWo * whCosWi) / denom * (1.0f - fr) * factor;
    }
}

float DielectricBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    if (approxDelta)
        return 0;

    const auto &[wo, wi, uv, spTemp] = intr;
    if (Math::sameHemisphere(wo, wi))
    {
        auto wh = glm::normalize(wo + wi);
        if (glm::dot(wo, wh) < 0)
            return 0;

        float fr = fresnelDielectric(Math::absDot(wh, wi), ior);
        return distrib.pdf(wh, wo) / (4.0f * Math::absDot(wh, wo)) * fr;
    }
    else
    {
        float eta = wo.z > 0 ? ior : 1.0f / ior;
        auto wh = glm::normalize(wo + wi * eta);
        if (Math::sameHemisphere(wh, wo, wi))
            return 0;

        float trans = 1.0f - fresnelDielectric(Math::absDot(wh, wo), eta);
        float dHdWi = Math::absDot(wh, wi) / Math::square(glm::dot(wh, wo) + eta * glm::dot(wh, wi));
        return distrib.pdf(wh, wo) * dHdWi * trans;
    }
}

std::optional<BSDFSample> DielectricBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    auto &wo = intr.wo;
    if (approxDelta)
    {
        float refl = fresnelDielectric(wo.z, ior);
        float trans = 1 - refl;

        if (u.x < refl)
        {
            Vec3f wi = { -wo.x, -wo.y, wo.z };
            return BSDFSample(wi, baseColor * refl, refl, BSDFType::Delta | BSDFType::Reflection);
        }
        else
        {
            float eta = (wo.z > 0.0f) ? ior : 1.0f / ior;
            Vec3f wi;
            bool refr = refract(wi, wo, ior);
            if (!refr)
                return std::nullopt;

            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;
            return BSDFSample(wi, baseColor * factor * trans, trans, BSDFType::Delta | BSDFType::Transmission, eta);
        }
    }
    else
    {
        Vec3f wh = distrib.sampleWm(wo, { u.y, u.z });
        if (wh.z < 0.0f)
            wh = -wh;
        float refl = fresnelDielectric(glm::dot(wh, wo), ior);
        float trans = 1.0f - refl;

        if (u.x < refl)
        {
            auto wi = -glm::reflect(wo, wh);
            if (!Math::sameHemisphere(wo, wi))
                return std::nullopt;

            float p = distrib.pdf(wh, wo) / (4.0f * Math::absDot(wh, wo));
            float hCosWo = Math::absDot(wh, wo);
            float hCosWi = Math::absDot(wh, wi);

            Spectrum r = (hCosWo * hCosWi < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * distrib.d(wh) * distrib.g(wo, wi) / (4.0f * hCosWo * hCosWi);

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(wi, r * refl, p * refl, BSDFType::Glossy | BSDFType::Reflection);
        }
        else
        {
            float eta = (glm::dot(wh, wo) > 0.0f) ? ior : 1.0f / ior;

            Vec3f wi;
            bool refr = refract(wi, wo, wh, ior);
            if (!refr || Math::sameHemisphere(wo, wi) || glm::abs(wi.z) < 1e-10f)
                return std::nullopt;

            float hCosWo = Math::absDot(wh, wo);
            float hCosWi = Math::absDot(wh, wi);

            float sqrtDenom = glm::dot(wh, wo) + eta * glm::dot(wh, wi);
            float denom = sqrtDenom * sqrtDenom;
            float dHdWi = hCosWi / denom;
            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

            denom *= glm::abs(wi.z) * glm::abs(wo.z);

            Spectrum r = (denom < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * glm::abs(distrib.d(wh) * distrib.g(wo, wi) * hCosWo * hCosWi) / denom * factor;

            float p = distrib.pdf(wh, wo) * dHdWi;

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(wi, r * trans, p * trans, BSDFType::Glossy | BSDFType::Transmission, eta);
        }
    }
}