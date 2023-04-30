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

float FresnelDielectric(float cosTi, float eta)
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

Spectrum DielectricBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const
{
    if (approxDelta)
        return Spectrum(0.0f);

    auto wh = glm::normalize(wo + wi);
    float whCosWo = Math::absDot(wh, wo);
    float whCosWi = Math::absDot(wh, wi);

    if (Math::sameHemisphere(wo, wi))
    {
        float fr = FresnelDielectric(Math::absDot(wh, wi), ior);
        return (whCosWo * whCosWi < 1e-7f) ? Spectrum(0.0f) : baseColor * distrib.d(wh) * distrib.g(wo, wi) / (4.0f * whCosWo * whCosWi) * fr;
    }
    else
    {
        float eta = wi.z > 0 ? ior : 1.0f / ior;
        float sqrtDenom = glm::dot(wh, wo) + eta * glm::dot(wh, wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = whCosWi / denom;

        denom *= glm::abs(wi.z * wo.z);
        float fr = FresnelDielectric(glm::dot(wh, wi), eta);
        float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

        return (denom < 1e-7f) ?
            Spectrum(0.0f) :
            baseColor * glm::abs(distrib.d(wh) * distrib.g(wo, wi) * whCosWo * whCosWi) / denom * (1.0f - fr) * factor;
    }
}

float DielectricBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const
{
    if (approxDelta)
        return 0;

    if (Math::sameHemisphere(wo, wi))
    {
        auto wh = glm::normalize(wo + wi);
        if (glm::dot(wo, wh) < 0)
            return 0;

        float fr = params.component.hasType(BSDFType::Reflection) ? FresnelDielectric(Math::absDot(wh, wi), ior) : 0;
        float tr = params.component.hasType(BSDFType::Transmission) ? 1.f - fr : 0;
        return distrib.pdf(wh, wo) / (4.0f * Math::absDot(wh, wo)) * fr / (fr + tr);
    }
    else
    {
        float eta = wo.z > 0 ? ior : 1.0f / ior;
        auto wh = glm::normalize(wo + wi * eta);
        if (Math::sameHemisphere(wh, wo, wi))
            return 0;

        float fr = params.component.hasType(BSDFType::Reflection) ? FresnelDielectric(Math::absDot(wh, wo), eta) : 0;
        float tr = params.component.hasType(BSDFType::Transmission) ? 1.f - fr : 0;
        float dHdWi = Math::absDot(wh, wi) / Math::square(glm::dot(wh, wo) + eta * glm::dot(wh, wi));
        return distrib.pdf(wh, wo) * dHdWi * tr / (fr + tr);
    }
}

std::optional<BSDFSample> DielectricBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler, BSDFType component) const
{
    if (!component.hasType(BSDFType::Reflection) && !component.hasType(BSDFType::Transmission))
        return std::nullopt;

    if (approxDelta)
    {
        float refl = component.hasType(BSDFType::Reflection) ? FresnelDielectric(wo.z, ior) : 0;
        float trans = component.hasType(BSDFType::Transmission) ? 1 - refl : 0;

        float fr = refl / (refl + trans);

        if (sampler->get1() < refl)
        {
            Vec3f wi = { -wo.x, -wo.y, wo.z };
            return BSDFSample(wi, baseColor * fr, fr, BSDFType::Delta | BSDFType::Reflection);
        }
        else
        {
            float eta = (wo.z > 0.0f) ? ior : 1.0f / ior;
            Vec3f wi;
            bool refr = refract(wi, wo, ior);
            if (!refr)
                return std::nullopt;

            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;
            return BSDFSample(wi, baseColor * factor * (1.f - fr), 1.f - fr, BSDFType::Delta | BSDFType::Transmission, eta);
        }
    }
    else
    {
        Vec3f wh = distrib.sampleWm(wo, sampler->get2());
        if (wh.z < 0.0f)
            wh = -wh;

        float refl = component.hasType(BSDFType::Reflection) ? FresnelDielectric(glm::dot(wh, wo), ior) : 0;
        float trans = component.hasType(BSDFType::Transmission) ? 1 - refl : 0;
        float fr = refl / (refl + trans);

        if (sampler->get1() < fr)
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
            return BSDFSample(wi, r * fr, p * fr, BSDFType::Glossy | BSDFType::Reflection);
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
            return BSDFSample(wi, r * (1.f - fr), p * (1.f - fr), BSDFType::Glossy | BSDFType::Transmission, eta);
        }
    }
}