#include "Core/BSDF.h"

Spectrum DisneyDiffuse::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);
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

float DisneyDiffuse::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    return intr.wi.z * Math::PiInv;
}

std::optional<BSDFSample> DisneyDiffuse::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    Vec3f wi = Math::sampleHemisphereCosine({ u.y, u.z });
    return BSDFSample(wi, baseColor * Math::PiInv, wi.z * Math::PiInv, BSDFType::Diffuse | BSDFType::Reflection);
}

Spectrum DisneyMetal::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);
    Vec3f h = glm::normalize(wo + wi);

    if (cosWo < 1e-10f || cosWi < 1e-10f)
        return Spectrum(0.0f);

    Spectrum f = schlickF(Math::absDot(h, wo), baseColor);
    float d = distrib.d(h);
    float g = distrib.g(wo, wi);

    float denom = 4.0f * cosWi * cosWo;
    if (denom < 1e-7f)
        return Spectrum(0.0f);
    return f * d * g / denom;
}

float DisneyMetal::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    Vec3f h = glm::normalize(wo + wi);
    return distrib.pdf(h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> DisneyMetal::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    Vec3f h = distrib.sampleWm(intr.wo, { u.y, u.z });
    Vec3f wi = glm::reflect(-intr.wo, h);
    if (wi.z < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, bsdf(newIntr, mode), pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection);
}

Spectrum DisneyClearcoat::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    auto h = glm::normalize(wo + wi);
    float cosWo = Math::saturate(wo.z);
    float cosWi = Math::saturate(wi.z);

    if (cosWo < 1e-6f || cosWi < 1e-6f)
        return Spectrum(0.0f);

    Spectrum r0(0.04f);
    Spectrum f = schlickF(Math::absDot(h, wo), r0);
    float d = distrib.d(h);
    float g = schlickG(cosWi, alpha) * schlickG(cosWi, alpha);

    float denom = 4.0f * cosWo * cosWi;
    if (denom < 1e-7f)
        return Spectrum(0.0f);
    return f * d * g / denom;
}

float DisneyClearcoat::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    auto h = glm::normalize(wo + wi);
    return distrib.pdf(h, wo) / (4.0f * glm::dot(h, wo));
}

std::optional<BSDFSample> DisneyClearcoat::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    auto h = distrib.sampleWm(intr.wo, { u.y, u.z });
    auto wi = glm::reflect(-intr.wo, h);

    if (wi.z < 0.0f)
        return std::nullopt;
    SurfaceIntr newIntr = intr;
    newIntr.wi = wi;
    return BSDFSample(wi, bsdf(newIntr, mode), pdf(newIntr, mode), BSDFType::Glossy | BSDFType::Reflection);
}

Spectrum DisneySheen::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    const auto &[wo, wi, uv, spTemp] = intr;
    Vec3f h = glm::normalize(wi + wo);
    float lum = Math::luminance(baseColor);
    Spectrum tintColor = lum > 0 ? baseColor / lum : Spectrum(1.0f);
    Spectrum sheenColor = Math::lerp(Spectrum(1.0f), tintColor, tint);
    return sheenColor * schlickW(Math::absDot(h, wo));
}

float DisneySheen::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    return intr.wi.z * Math::PiInv;
}

std::optional<BSDFSample> DisneySheen::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    Vec3f wi = Math::sampleHemisphereCosine({ u.y, u.z });
    return BSDFSample(wi, bsdf({ intr.wo, wi, intr.uv }, mode), wi.z * Math::PiInv, BSDFType::Diffuse | BSDFType::Reflection);
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

    std::vector<float> w(4);
    std::copy(weights, weights + 4, w.data());
    piecewiseSampler = Piecewise1D(w);
}

Spectrum DisneyBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const
{
    Spectrum r(0.0f);
    r += diffuse.bsdf(intr, mode) * weights[0];
    r += metal.bsdf(intr, mode) * weights[1];
    r += clearcoat.bsdf(intr, mode) * weights[2];
    r += dielectric.bsdf(intr, mode) * weights[3];
    r += sheen.bsdf(intr, mode) * weights[4];
    return r;
}

float DisneyBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const
{
    float p = 0.0f;
    p += diffuse.pdf(intr, mode) * weights[0];
    p += metal.pdf(intr, mode) * weights[1];
    p += clearcoat.pdf(intr, mode) * weights[2];
    p += dielectric.pdf(intr, mode) * weights[3];
    p += sheen.pdf(intr, mode) * weights[4];
    return p;
}

std::optional<BSDFSample> DisneyBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const
{
    int comp = piecewiseSampler.sample(intr.sampler->get2());
    std::optional<BSDFSample> s;

    switch (comp) {
    case 0:
        s = diffuse.sample(intr, u, mode);
        break;
    case 1:
        s = metal.sample(intr, u, mode);
        break;
    case 2:
        s = clearcoat.sample(intr, u, mode);
        break;
    case 3:
        s = dielectric.sample(intr, u, mode);
        break;
    case 4:
        s = sheen.sample(intr, u, mode);
        break;
    }
    if (s) {
        s->pdf *= weights[comp];
        return s;
    }
    return std::nullopt;
}