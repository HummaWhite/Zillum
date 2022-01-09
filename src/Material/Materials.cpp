#include "Materials.h"

Vec3f Lambertian::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return albedo * Math::PiInv;
}

float Lambertian::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return glm::dot(Wi, N) * Math::PiInv;
}

Sample Lambertian::getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto [Wi, pdf] = Math::sampleHemisphereCosine(N, u2);
    return Sample(Wi, pdf, BXDF::Diffuse);
}

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

Vec3f Clearcoat::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);

    float NoWo = Math::satDot(N, Wo);
    float NoWi = Math::satDot(N, Wi);

    float D = distrib.d(N, H);
    auto F = Microfacet::schlickF(Math::absDot(H, Wo), Vec3f(0.04f));
    float G = Microfacet::smithG(N, Wo, Wi, 0.25f);

    float denom = 4.0f * NoWo * NoWi;
    if (denom < 1e-7f)
        return Vec3f(0.0f);

    return F * D * G * weight / denom;
}

float Clearcoat::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);
    return distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
}

Sample Clearcoat::getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto H = distrib.sampleWm(N, Wo, u2);
    auto Wi = glm::reflect(-Wo, H);

    if (glm::dot(N, Wi) < 0.0f)
        return Sample();

    return Sample(Wi, pdf(N, Wo, Wi, mode), BXDF::GlosRefl);
}

Vec3f Dielectric::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    if (approxDelta)
        return Vec3f(0.0f);

    auto H = glm::normalize(Wo + Wi);
    float HoWo = Math::absDot(H, Wo);
    float HoWi = Math::absDot(H, Wi);

    if (Math::sameHemisphere(N, Wo, Wi))
    {
        float refl = fresnelDielectric(Math::absDot(H, Wi), ior);
        return (HoWo * HoWi < 1e-7f) ? Vec3f(0.0f) : tint * distrib.d(N, H) * distrib.g(N, Wo, Wi) / (4.0f * HoWo * HoWi) * refl;
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
            Vec3f(0.0f) :
            tint * glm::abs(distrib.d(N, H) * distrib.g(N, Wo, Wi) * HoWo * HoWi) / denom * (1.0f - refl) * factor;
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

SampleWithBsdf Dielectric::sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    if (approxDelta)
    {
        float refl = fresnelDielectric(glm::dot(N, Wo), ior), trans = 1 - refl;

        if (u1 < refl)
        {
            Vec3f Wi = -glm::reflect(Wo, N);
            return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
        }
        else
        {
            float eta = (glm::dot(N, Wo) > 0.0f) ? ior : 1.0f / ior;
            Vec3f Wi;
            bool refr = refract(Wi, Wo, N, ior);
            if (!refr)
                return INVALID_BSDF_SAMPLE;

            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;
            return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecTrans, eta), tint * factor);
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
                return INVALID_BSDF_SAMPLE;

            float p = distrib.pdf(N, H, Wo) / (4.0f * Math::absDot(H, Wo));
            float HoWo = Math::absDot(H, Wo);
            float HoWi = Math::absDot(H, Wi);

            Vec3f r = (HoWo * HoWi < 1e-7f) ?
                Vec3f(0.0f) :
                tint * distrib.d(N, H) * distrib.g(N, Wo, Wi) / (4.0f * HoWo * HoWi);

            if (Math::isNan(p))
                p = 0.0f;
            return SampleWithBsdf(Sample(Wi, p, BXDF::GlosRefl), r);
        }
        else
        {
            float eta = (glm::dot(H, Wo) > 0.0f) ? ior : 1.0f / ior;

            Vec3f Wi;
            bool refr = refract(Wi, Wo, H, ior);
            if (!refr)
                return INVALID_BSDF_SAMPLE;
            if (Math::sameHemisphere(N, Wo, Wi))
                return INVALID_BSDF_SAMPLE;
            if (Math::absDot(N, Wi) < 1e-10f)
                return INVALID_BSDF_SAMPLE;

            float HoWo = Math::absDot(H, Wo);
            float HoWi = Math::absDot(H, Wi);

            float sqrtDenom = glm::dot(H, Wo) + eta * glm::dot(H, Wi);
            float denom = sqrtDenom * sqrtDenom;
            float dHdWi = HoWi / denom;
            float factor = (mode == TransportMode::Radiance) ? Math::square(1.0f / eta) : 1.0f;

            denom *= Math::absDot(N, Wi) * Math::absDot(N, Wo);

            Vec3f r = (denom < 1e-7f) ?
                Vec3f(0.0f) :
                tint * glm::abs(distrib.d(N, H) * distrib.g(N, Wo, Wi) * HoWo * HoWi) / denom * factor;

            float p = distrib.pdf(N, H, Wo) * dHdWi;

            if (Math::isNan(p))
                p = 0.0f;
            return SampleWithBsdf(Sample(Wi, p, BXDF::GlosTrans, eta), r);
        }
    }
}

Vec3f ThinDielectric::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return Vec3f(0.0f);
}

SampleWithBsdf ThinDielectric::sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    float refl = fresnelDielectric(glm::dot(N, Wo), ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f)
    {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }

    if (u1 < refl)
    {
        auto Wi = glm::reflect(-Wo, N);
        return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
    }
    else
    {
        return SampleWithBsdf(Sample(-Wo, 1.0f, BXDF::SpecTrans), tint);
    }
}