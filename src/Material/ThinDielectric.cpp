#include "Core/BSDF.h"

Spectrum ThinDielectric::bsdf(const SurfaceIntr &intr, TransportMode mode) {
    return Spectrum(0.0f);
}

std::optional<BSDFSample> ThinDielectric::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) {
    float refl = fresnelDielectric(glm::dot(intr.n, intr.wo), ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f) {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }
    return (u.x < refl) ?
        BSDFSample(glm::reflect(-intr.wo, intr.n), baseColor, 1.0f, BSDFType::Delta | BSDFType::Reflection) :
        BSDFSample(-intr.wo, baseColor, 1.0f, BSDFType::Delta | BSDFType::Transmission);
}