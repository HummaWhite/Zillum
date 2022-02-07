#include "../../include/Core/Material.h"

bool refract(Vec3f &Wt, const Vec3f &Wi, const Vec3f &N, float eta)
{
    float cosTi = glm::dot(N, Wi);
    if (cosTi < 0)
        eta = 1.0f / eta;
    float sin2Ti = glm::max<float>(0.0f, 1.0f - cosTi * cosTi);
    float sin2Tt = sin2Ti / (eta * eta);

    if (sin2Tt >= 1.0f)
        return false;

    float cosTt = glm::sqrt(1.0f - sin2Tt);
    if (cosTi < 0)
        cosTt = -cosTt;
    Wt = glm::normalize(-Wi / eta + N * (cosTi / eta - cosTt));
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

Spectrum Dielectric::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    if (approxDelta)
        return Spectrum(0.0f);

    auto H = glm::normalize(Wo + Wi);
    float HoWo = Math::absDot(H, Wo);
    float HoWi = Math::absDot(H, Wi);

    if (Math::sameHemisphere(N, Wo, Wi))
    {
        float refl = fresnelDielectric(Math::absDot(H, Wi), ior);
        return (HoWo * HoWi < 1e-7f) ? Spectrum(0.0f) : baseColor * distrib.d(N, H) * distrib.g(N, Wo, Wi) / (4.0f * HoWo * HoWi) * refl;
    }
    else
    {
        float eta = glm::dot(N, Wi) > 0 ? ior : 1.0f / ior;
        float sqrtDenom = glm::dot(H, Wo) + eta * glm::dot(H, Wi);
        float denom = sqrtDenom * sqrtDenom;
        float dHdWi = HoWi / denom;

        denom *= Math::absDot(N, Wi) * Math::absDot(N, Wo);
        float refl = fresnelDielectric(glm::dot(H, Wi), eta);
        float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

        return (denom < 1e-7f) ?
            Spectrum(0.0f) :
            baseColor * glm::abs(distrib.d(N, H) * distrib.g(N, Wo, Wi) * HoWo * HoWi) / denom * (1.0f - refl) * factor;
    }
}

float Dielectric::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    if (approxDelta)
        return 0;

    if (Math::sameHemisphere(N, Wo, Wi))
    {
        auto H = glm::normalize(Wo + Wi);
        if (glm::dot(Wo, H) < 0)
            return 0;

        float refl = fresnelDielectric(Math::absDot(H, Wi), ior);
        return distrib.pdf(N, H, Wo) / (4.0f * Math::absDot(H, Wo)) * refl;
    }
    else
    {
        float eta = glm::dot(N, Wo) > 0 ? ior : 1.0f / ior;
        auto H = glm::normalize(Wo + Wi * eta);
        if (Math::sameHemisphere(H, Wo, Wi))
            return 0;

        float trans = 1.0f - fresnelDielectric(Math::absDot(H, Wo), eta);
        float dHdWi = Math::absDot(H, Wi) / Math::square(glm::dot(H, Wo) + eta * glm::dot(H, Wi));
        return distrib.pdf(N, H, Wo) * dHdWi * trans;
    }
}

std::optional<BSDFSample> Dielectric::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    if (approxDelta)
    {
        float refl = fresnelDielectric(glm::dot(N, Wo), ior), trans = 1 - refl;

        if (u1 < refl)
        {
            Vec3f Wi = -glm::reflect(Wo, N);
            return BSDFSample(Wi, 1.0f, BXDF::SpecRefl, baseColor);
        }
        else
        {
            float eta = (glm::dot(N, Wo) > 0.0f) ? ior : 1.0f / ior;
            Vec3f Wi;
            bool refr = refract(Wi, Wo, N, ior);
            if (!refr)
                return std::nullopt;

            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;
            return BSDFSample(Wi, 1.0f, BXDF::SpecTrans, baseColor * factor, eta);
        }
    }
    else
    {
        Vec3f H = distrib.sampleWm(N, Wo, u2);
        if (glm::dot(N, H) < 0.0f)
            H = -H;
        float refl = fresnelDielectric(glm::dot(H, Wo), ior);
        float trans = 1.0f - refl;

        if (u1 < refl)
        {
            auto Wi = -glm::reflect(Wo, H);
            if (!Math::sameHemisphere(N, Wo, Wi))
                return std::nullopt;

            float p = distrib.pdf(N, H, Wo) / (4.0f * Math::absDot(H, Wo));
            float HoWo = Math::absDot(H, Wo);
            float HoWi = Math::absDot(H, Wi);

            Spectrum r = (HoWo * HoWi < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * distrib.d(N, H) * distrib.g(N, Wo, Wi) / (4.0f * HoWo * HoWi);

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(Wi, p, BXDF::GlosRefl, r);
        }
        else
        {
            float eta = (glm::dot(H, Wo) > 0.0f) ? ior : 1.0f / ior;

            Vec3f Wi;
            bool refr = refract(Wi, Wo, H, ior);
            if (!refr || Math::sameHemisphere(N, Wo, Wi) || Math::absDot(N, Wi) < 1e-10f)
                return std::nullopt;

            float HoWo = Math::absDot(H, Wo);
            float HoWi = Math::absDot(H, Wi);

            float sqrtDenom = glm::dot(H, Wo) + eta * glm::dot(H, Wi);
            float denom = sqrtDenom * sqrtDenom;
            float dHdWi = HoWi / denom;
            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

            denom *= Math::absDot(N, Wi) * Math::absDot(N, Wo);

            Spectrum r = (denom < 1e-7f) ?
                Spectrum(0.0f) :
                baseColor * glm::abs(distrib.d(N, H) * distrib.g(N, Wo, Wi) * HoWo * HoWi) / denom * factor;

            float p = distrib.pdf(N, H, Wo) * dHdWi;

            if (Math::isNan(p))
                p = 0.0f;
            return BSDFSample(Wi, p, BXDF::GlosTrans, r, eta);
        }
    }
}