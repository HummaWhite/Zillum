#include "../../include/Core/Material.h"

Spectrum Mirror::bsdf(const SurfaceIntr &intr, TransportMode mode) {
    return Spectrum(0.0f);
}

float Mirror::pdf(const SurfaceIntr &intr, TransportMode mode) {
    return 0.0f;
}

std::optional<BSDFSample> Mirror::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) {
    Vec3f wi = glm::reflect(-intr.wo, intr.n);
    return BSDFSample(wi, 1.0f, BXDF::SpecRefl, baseColor->get(intr.uv));
}