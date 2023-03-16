#include "Core/BSDF.h"

Spectrum DisneyDiffuse::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    float cosWo = Math::satDot(n, wo);
    float cosWi = Math::satDot(n, wi);
    Vec3f h = glm::normalize(wo + wi);
    float hCosWi = glm::dot(h, wi);
    float hCosWi2 = hCosWi * hCosWi;

    if (cosWo < 1e-10f || cosWi < 1e-10f)
        return Spectrum(0.0f);

    float fi = schlickW(cosWi);
    float fo = schlickW(cosWo);
    Spectrum fd90(0.5f + 2.0f * roughness * hCosWi2);
    Spectrum fd = Math::lerp(Spectrum(1.0f), fd90, fi) * Math::lerp(Spectrum(1.0f), fd90, fo);
    Spectrum baseDiffuse = baseColor * fd * Math::PiInv;

    Spectrum fss90(roughness * hCosWi2);
    Spectrum fss = Math::lerp(Spectrum(1.0f), fss90, fi) * Math::lerp(Spectrum(1.0f), fss90, fo);
    Spectrum ss = baseColor * Math::PiInv * 1.25f *
               (fss * (1.0f / (cosWi + cosWo) - 0.5f) + 0.5f);

    Spectrum diffuse = Math::lerp(baseDiffuse, ss, subsurface);
    return diffuse;
}

float DisneyDiffuse::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    return glm::dot(wi, n) * Math::PiInv;
}

std::optional<BSDFSample> DisneyDiffuse::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    auto [wi, pdf] = Math::sampleHemisphereCosine(intr.n, { u.y, u.z });
    return BSDFSample(wi, pdf, BSDFType::Diffuse | BSDFType::Reflection, baseColor * Math::PiInv);
}

Spectrum DisneyMetal::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    float cosWo = Math::satDot(n, wo);
    float cosWi = Math::satDot(n, wi);
    Vec3f h = glm::normalize(wo + wi);

    if (cosWo < 1e-10f || cosWi < 1e-10f)
        return Spectrum(0.0f);

    Spectrum f = schlickF(Math::absDot(h, wo), baseColor);
    float d = distrib.d(n, h);
    float g = distrib.g(n, wo, wi);

    float denom = 4.0f * cosWi * cosWo;
    if (denom < 1e-7f)
        return Spectrum(0.0f);
    return f * d * g / denom;
}

float DisneyMetal::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    float cosWi = glm::dot(n, wi);
    Vec3f h = glm::normalize(wo + wi);
    return distrib.pdf(n, h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> DisneyMetal::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    Vec3f h = distrib.sampleWm(intr.n, intr.wo, { u.y, u.z });
    Vec3f wi = glm::reflect(-intr.wo, h);
    if (glm::dot(intr.n, wi) < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection, bsdf(newIntr, mode));
}

Spectrum DisneyClearcoat::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    auto h = glm::normalize(wo + wi);
    float cosWo = Math::satDot(n, wo);
    float cosWi = Math::satDot(n, wi);

    if (cosWo < 1e-6f || cosWi < 1e-6f)
        return Spectrum(0.0f);

    Spectrum r0(0.04f);
    Spectrum f = schlickF(Math::absDot(h, wo), r0);
    float d = distrib.d(n, h);
    float g = schlickG(cosWi, alpha) * schlickG(cosWi, alpha);

    float denom = 4.0f * cosWo * cosWi;
    if (denom < 1e-7f)
        return Spectrum(0.0f);
    return f * d * g / denom;
}

float DisneyClearcoat::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    auto h = glm::normalize(wo + wi);
    return distrib.pdf(n, h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> DisneyClearcoat::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    auto h = distrib.sampleWm(intr.n, intr.wo, { u.y, u.z });
    auto wi = glm::reflect(-intr.wo, h);

    if (glm::dot(intr.n, wi) < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection, bsdf(newIntr, mode));
}

Spectrum DisneySheen::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    Vec3f h = glm::normalize(wi + wo);
    float lum = Math::luminance(baseColor);
    Spectrum tintColor = lum > 0 ? baseColor / lum : Spectrum(1.0f);
    Spectrum sheenColor = Math::lerp(Spectrum(1.0f), tintColor, tint);
    return sheenColor * schlickW(Math::absDot(h, wo));
}

float DisneySheen::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    const auto &[n, wo, wi, uv] = intr;
    return glm::dot(wi, n) * Math::PiInv;
}

std::optional<BSDFSample> DisneySheen::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    auto [wi, pdf] = Math::sampleHemisphereCosine(intr.n, { u.y, u.z });
    return BSDFSample(wi, pdf, BSDFType::Diffuse | BSDFType::Reflection, bsdf({ intr.n, intr.wo, wi, intr.uv }, mode));
}

// TODO: this implementation is not correct
DisneyBSDF::DisneyBSDF(
    const Spectrum &baseColor,
    float subsurface,
    float metallic,
    float roughness,
    float specular,
    float specularTint,
    float sheen,
    float sheenTint,
    float clearcoat,
    float clearcoatGloss,
    float transmission,
	float transmissionRoughness,
    float ior
) :
    diffuse(baseColor, roughness, subsurface),
    metal(baseColor, roughness),
    clearcoat(clearcoatGloss),
    sheen(baseColor, sheenTint),
    dielectric(baseColor, transmissionRoughness, ior),
    BSDF(BSDFType::Delta | BSDFType::Diffuse | BSDFType::Glossy | BSDFType::Reflection | BSDFType::Transmission)
{
    weights[0] = (1.0f - metallic) * (1.0f - transmission);
    weights[1] = (1.0f - transmission * (1.0f - metallic));
    weights[2] = 0.25f * clearcoat;
    weights[3] = (1.0f - metallic) * transmission;
    weights[4] = (1.0f - metallic) * sheen;

    components[0] = &diffuse;
    components[1] = &metal;
    components[2] = &this->clearcoat;
    components[3] = &dielectric;
    components[4] = &this->sheen;

    std::vector<float> w(4);
    std::copy(weights, weights + 4, w.data());
    piecewiseSampler = Piecewise1D(w);
}

Spectrum DisneyBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode)
{
    Spectrum r(0.0f);
    for (int i = 0; i < 5; i++)
        r += components[i]->bsdf(intr, mode) * weights[i];
    return r;
}

float DisneyBSDF::pdf(const SurfaceIntr &intr, TransportMode mode)
{
    float p = 0.0f;
    for (int i = 0; i < 4; i++)
        p += components[i]->pdf(intr, mode) * weights[i];
    p /= piecewiseSampler.sum();
    return p;
}

std::optional<BSDFSample> DisneyBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode)
{
    // TODO: here needs two extra samples, causing sampling of each path not aligned
    int comp = piecewiseSampler.sample({ uniformFloat(), uniformFloat() });
    return components[comp]->sample(intr, u, mode);
}