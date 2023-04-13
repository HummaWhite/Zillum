#include "Core/BSDF.h"

bool refract(Vec3f &wt, const Vec3f &wi, const Vec3f &n, float eta)
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

Spectrum DielectricBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    if (approxDelta)
        return Spectrum(0.0f);

    const auto &[n, wo, wi, uv, spTemp] = intr;
    auto h = glm::normalize(wo + wi);
    float hCosWo = Math::absDot(h, wo);
    float hCosWi = Math::absDot(h, wi);

    if (Math::sameHemisphere(n, wo, wi))
    {
        float refl = fresnelDielectric(Math::absDot(h, wi), ior);
        return (hCosWo * hCosWi < 1e-7f) ? Spectrum(0.0f) : baseColor * distrib.d(n, h) * distrib.g(n, wo, wi) / (4.0f * hCosWo * hCosWi) * refl;
    }
    else
    {
        float eta = glm::dot(n, wi) > 0 ? ior : 1.0f / ior;
        float sqrtDenom = glm::dot(h, wo) + eta * glm::dot(h, wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = hCosWi / denom;

        denom *= Math::absDot(n, wi) * Math::absDot(n, wo);
        float refl = fresnelDielectric(glm::dot(h, wi), eta);
        float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

        return (denom < 1e-7f) ?
            Spectrum(0.0f) :
            baseColor * glm::abs(distrib.d(n, h) * distrib.g(n, wo, wi) * hCosWo * hCosWi) / denom * (1.0f - refl) * factor;
    }
}

float DielectricBSDF::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    if (approxDelta)
        return 0;

    const auto &[n, wo, wi, uv, spTemp] = intr;
    if (Math::sameHemisphere(n, wo, wi))
    {
        auto h = glm::normalize(wo + wi);
        if (glm::dot(wo, h) < 0)
            return 0;

        float refl = fresnelDielectric(Math::absDot(h, wi), ior);
        return distrib.pdf(n, h, wo) / (4.0f * Math::absDot(h, wo)) * refl;
    }
    else
    {
        float eta = glm::dot(n, wo) > 0 ? ior : 1.0f / ior;
        auto h = glm::normalize(wo + wi * eta);
        if (Math::sameHemisphere(h, wo, wi))
            return 0;

        float trans = 1.0f - fresnelDielectric(Math::absDot(h, wo), eta);
        float dHdWi = Math::absDot(h, wi) / Math::square(glm::dot(h, wo) + eta * glm::dot(h, wi));
        return distrib.pdf(n, h, wo) * dHdWi * trans;
    }
}

std::optional<BSDFSample> DielectricBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    auto &n = intr.n;
    auto &wo = intr.wo;
    if (approxDelta)
    {
        float refl = fresnelDielectric(glm::dot(n, wo), ior);
        float trans = 1 - refl;

        if (u.x < refl)
        {
            Vec3f wi = -glm::reflect(wo, n);
            return BSDFSample(wi, baseColor * refl, refl, BSDFType::Delta | BSDFType::Reflection);
        }
        else
        {
            float eta = (glm::dot(n, wo) > 0.0f) ? ior : 1.0f / ior;
            Vec3f wi;
            bool refr = refract(wi, wo, n, ior);
            if (!refr)
                return std::nullopt;

            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;
            return BSDFSample(wi, baseColor * factor * trans, trans, BSDFType::Delta | BSDFType::Transmission, eta);
        }
    }
    else
    {
        Vec3f h = distrib.sampleWm(n, wo, { u.y, u.z });
        if (glm::dot(n, h) < 0.0f)
            h = -h;
        float refl = fresnelDielectric(glm::dot(h, wo), ior);
        float trans = 1.0f - refl;

        if (u.x < refl)
        {
            auto wi = -glm::reflect(wo, h);
            if (!Math::sameHemisphere(n, wo, wi))
                return std::nullopt;

            float p = distrib.pdf(n, h, wo) / (4.0f * Math::absDot(h, wo));
            float hCosWo = Math::absDot(h, wo);
            float hCosWi = Math::absDot(h, wi);

            Spectrum r = (hCosWo * hCosWi < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * distrib.d(n, h) * distrib.g(n, wo, wi) / (4.0f * hCosWo * hCosWi);

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(wi, r * refl, p * refl, BSDFType::Glossy | BSDFType::Reflection);
        }
        else
        {
            float eta = (glm::dot(h, wo) > 0.0f) ? ior : 1.0f / ior;

            Vec3f wi;
            bool refr = refract(wi, wo, h, ior);
            if (!refr || Math::sameHemisphere(n, wo, wi) || Math::absDot(n, wi) < 1e-10f)
                return std::nullopt;

            float hCosWo = Math::absDot(h, wo);
            float hCosWi = Math::absDot(h, wi);

            float sqrtDenom = glm::dot(h, wo) + eta * glm::dot(h, wi);
            float denom = sqrtDenom * sqrtDenom;
            float dHdWi = hCosWi / denom;
            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

            denom *= Math::absDot(n, wi) * Math::absDot(n, wo);

            Spectrum r = (denom < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * glm::abs(distrib.d(n, h) * distrib.g(n, wo, wi) * hCosWo * hCosWi) / denom * factor;

            float p = distrib.pdf(n, h, wo) * dHdWi;

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(wi, r * trans, p * trans, BSDFType::Glossy | BSDFType::Transmission, eta);
        }
    }
}