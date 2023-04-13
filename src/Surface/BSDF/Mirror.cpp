#include "Core/BSDF.h"

Spectrum MirrorBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) {
    return Spectrum(0.0f);
}

float MirrorBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) {
    return 0.0f;
}

std::optional<BSDFSample> MirrorBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) {
    Vec3f wi = glm::reflect(-intr.wo, intr.n);
    return BSDFSample(wi, baseColor.get(intr.uv), 1.0f, BSDFType::Delta | BSDFType::Reflection);
}