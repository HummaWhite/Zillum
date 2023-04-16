#include "Core/BSDF.h"

Spectrum MirrorBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) const {
    return Spectrum(0.0f);
}

float MirrorBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) const {
    return 0.0f;
}

std::optional<BSDFSample> MirrorBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) const {
    Vec3f wi = { -intr.wo.x, -intr.wo.y, intr.wo.z };
    return BSDFSample(wi, baseColor.get(intr.uv), 1.0f, BSDFType::Delta | BSDFType::Reflection);
}