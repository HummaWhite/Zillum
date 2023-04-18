#include "Core/BSDF.h"

Spectrum ThinDielectricBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const {
    return Spectrum(0.0f);
}

std::optional<BSDFSample> ThinDielectricBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const {
    float refl = FresnelDielectric(intr.wo.z, ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f) {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }
    return (u.x < refl) ?
        BSDFSample({ -intr.wo.x, -intr.wo.y, intr.wo.z }, baseColor, 1.0f, BSDFType::Delta | BSDFType::Reflection) :
        BSDFSample(-intr.wo, baseColor, 1.0f, BSDFType::Delta | BSDFType::Transmission);
}