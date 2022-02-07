#include "../../include/Core/Material.h"

Spectrum ThinDielectric::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return Spectrum(0.0f);
}

std::optional<BSDFSample> ThinDielectric::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    float refl = fresnelDielectric(glm::dot(N, Wo), ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f)
    {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }
    return (u1 < refl) ?
        BSDFSample(glm::reflect(-Wo, N), 1.0f, BXDF::SpecRefl, baseColor) :
        BSDFSample(-Wo, 1.0f, BXDF::SpecTrans, baseColor);
}