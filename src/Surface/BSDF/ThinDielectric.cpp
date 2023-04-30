#include "Core/BSDF.h"

Spectrum ThinDielectricBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Params params) const {
    return Spectrum(0.0f);
}

std::optional<BSDFSample> ThinDielectricBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler, BSDFType component) const {
    float refl = FresnelDielectric(wo.z, ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f) {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }
    return (sampler->get1() < refl) ?
        BSDFSample({ -wo.x, -wo.y, wo.z }, baseColor, 1.0f, BSDFType::Delta | BSDFType::Reflection) :
        BSDFSample(-wo, baseColor, 1.0f, BSDFType::Delta | BSDFType::Transmission);
}