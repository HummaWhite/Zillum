#include "Core/BSDF.h"

Spectrum LambertBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const {
    return albedo.get(intr.uv) * Math::PiInv;
}

float LambertBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const {
    return intr.wi.z * Math::PiInv;
}

std::optional<BSDFSample> LambertBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const {
    Vec3f wi = Math::sampleHemisphereCosine({ u.y, u.z });
    return BSDFSample(wi, albedo.get(intr.uv) * Math::PiInv, wi.z * Math::PiInv, BSDFType::Diffuse | BSDFType::Reflection);
}