#include "../../include/Core/Material.h"

Spectrum ThinDielectric::bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    return Spectrum(0.0f);
}

std::optional<BSDFSample> ThinDielectric::sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode)
{
    float refl = fresnelDielectric(glm::dot(n, wo), ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f)
    {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }
    return (u.x < refl) ?
        BSDFSample(glm::reflect(-wo, n), 1.0f, BXDF::SpecRefl, baseColor) :
        BSDFSample(-wo, 1.0f, BXDF::SpecTrans, baseColor);
}