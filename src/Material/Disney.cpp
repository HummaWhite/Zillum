#include "../../include/Core/Material.h"

Vec3f DisneyDiffuse::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    float NoWo = Math::satDot(N, Wo);
    float NoWi = Math::satDot(N, Wi);
    Vec3f H = glm::normalize(Wo + Wi);
    float HoWi = glm::dot(H, Wi);
    float HoWi2 = HoWi * HoWi;

    if (NoWo < 1e-10f || NoWi < 1e-10f)
        return Vec3f(0.0f);

    float Fi = schlickW(NoWi);
    float Fo = schlickW(NoWo);
    Vec3f Fd90(0.5f + 2.0f * roughness * HoWi2);
    Vec3f Fd = glm::mix(Vec3f(1.0f), Fd90, Fi) * glm::mix(Vec3f(1.0f), Fd90, Fo);
    Vec3f baseDiffuse = baseColor * Fd * Math::PiInv;

    Vec3f Fss90(roughness * HoWi2);
    Vec3f Fss = glm::mix(Vec3f(1.0f), Fss90, Fi) * glm::mix(Vec3f(1.0f), Fss90, Fo);
    Vec3f ss = baseColor * Math::PiInv * 1.25f *
               (Fss * (1.0f / (NoWi + NoWo) - 0.5f) + 0.5f);

    Vec3f diffuse = glm::mix(baseDiffuse, ss, subsurface);

    return diffuse;
}

float DisneyDiffuse::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return glm::dot(Wi, N) * Math::PiInv;
}

std::optional<BSDFSample> DisneyDiffuse::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto [Wi, pdf] = Math::sampleHemisphereCosine(N, u2);
    return BSDFSample(Wi, pdf, BXDF::Diffuse, baseColor * Math::PiInv);
}

Vec3f DisneyMetal::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    float NoWo = Math::satDot(N, Wo);
    float NoWi = Math::satDot(N, Wi);
    Vec3f H = glm::normalize(Wo + Wi);

    if (NoWo < 1e-10f || NoWi < 1e-10f)
        return Vec3f(0.0f);

    Vec3f F = schlickF(Math::absDot(H, Wo), baseColor);
    float D = distrib.d(N, H);
    float G = distrib.g(N, Wo, Wi);

    float denom = 4.0f * NoWi * NoWo;
    if (denom < 1e-7f)
        return Vec3f(0.0f);

    return F * D * G / denom;
}

float DisneyMetal::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    float NoWi = glm::dot(N, Wi);
    Vec3f H = glm::normalize(Wo + Wi);
    return distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
}

std::optional<BSDFSample> DisneyMetal::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    Vec3f H = distrib.sampleWm(N, Wo, u2);
    Vec3f Wi = glm::reflect(-Wo, H);

    float NoWi = glm::dot(N, Wi);
    if (NoWi < 0.0f)
        return std::nullopt;
    return BSDFSample(Wi, pdf(N, Wo, Wi, mode), BXDF::GlosRefl, bsdf(N, Wo, Wi, mode));
}

Vec3f DisneyClearcoat::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);
    float NoWo = Math::satDot(N, Wo);
    float NoWi = Math::satDot(N, Wi);

    if (NoWo < 1e-6f || NoWi < 1e-6f)
        return Vec3f(0.0f);

    const Vec3f R0(0.04f);
    Vec3f F = schlickF(Math::absDot(H, Wo), R0);
    float D = distrib.d(N, H);
    float G = schlickG(NoWi, alpha) * schlickG(NoWi, alpha);

    float denom = 4.0f * NoWo * NoWi;
    if (denom < 1e-7f)
        return Vec3f(0.0f);

    return F * D * G / denom;
}

float DisneyClearcoat::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    auto H = glm::normalize(Wo + Wi);
    return distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
}

std::optional<BSDFSample> DisneyClearcoat::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto H = distrib.sampleWm(N, Wo, u2);
    auto Wi = glm::reflect(-Wo, H);

    if (glm::dot(N, Wi) < 0.0f)
        return std::nullopt;
    return BSDFSample(Wi, pdf(N, Wo, Wi, mode), BXDF::GlosRefl, bsdf(N, Wo, Wi, mode));
}

Vec3f DisneySheen::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    Vec3f H = glm::normalize(Wi + Wo);
    float lum = Math::luminance(baseColor);
    Vec3f tintColor = lum > 0 ? baseColor / lum : Vec3f(1.0f);
    Vec3f sheenColor = glm::mix(Vec3f(1.0f), tintColor, tint);
    return sheenColor * schlickW(Math::absDot(H, Wo));
}

float DisneySheen::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return glm::dot(Wi, N) * Math::PiInv;
}

std::optional<BSDFSample> DisneySheen::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto [Wi, pdf] = Math::sampleHemisphereCosine(N, u2);
    return BSDFSample(Wi, pdf, BXDF::Diffuse, bsdf(N, Wo, Wi, mode));
}

DisneyBSDF::DisneyBSDF(
    const Vec3f &baseColor,
    float subsurface,
    float metallic,
    float roughness,
    float specular,
    float specularTint,
    float sheen,
    float sheenTint,
    float clearcoat,
    float clearcoatTint,
    float transmission,
    float ior
) :
    diffuse(baseColor, roughness, subsurface),
    metal(baseColor, roughness),
    clearcoat(clearcoat),
    sheen(baseColor, sheenTint),
    dielectric(baseColor, roughness, ior),
    Material(BXDF::Diffuse | BXDF::GlosRefl | BXDF::GlosTrans)
{
    weights[0] = (1.0f - metallic) * (1.0f - transmission);
    weights[1] = (1.0f - transmission * (1.0f - metallic));
    weights[2] = 0.25f * clearcoat;
    weights[3] = (1.0f - metallic) * transmission;

    components[0] = &diffuse;
    components[1] = &metal;
    components[2] = &this->clearcoat;
    components[3] = &dielectric;

    std::vector<float> w(4);
    std::copy(weights, weights + 4, w.data());
    piecewiseSampler = Piecewise1D(w);
}

Vec3f DisneyBSDF::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    Vec3f r(0.0f);
    for (int i = 0; i < 4; i++)
        r += components[i]->bsdf(N, Wo, Wi, mode) * weights[i];
    return r;
}

float DisneyBSDF::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    float p = 0.0f;
    for (int i = 0; i < 4; i++)
        p += components[i]->pdf(N, Wo, Wi, mode) * weights[i];
    p /= piecewiseSampler.sum();
    return p;
}

std::optional<BSDFSample> DisneyBSDF::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    int comp = piecewiseSampler.sample({ uniformFloat(), uniformFloat() });
    return components[comp]->sample(N, Wo, u1, u2, mode);
}