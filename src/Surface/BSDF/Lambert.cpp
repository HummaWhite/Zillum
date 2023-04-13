#include "Core/BSDF.h"

Spectrum LambertBSDF::bsdf(const SurfaceIntr &intr, TransportMode mode) {
    return albedo.get(intr.uv) * Math::PiInv;
}

float LambertBSDF::pdf(const SurfaceIntr &intr, TransportMode mode) {
    return glm::dot(intr.n, intr.wi) * Math::PiInv;
}

std::optional<BSDFSample> LambertBSDF::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) {
    auto [wi, pdf] = Math::sampleHemisphereCosine(intr.n, { u.y, u.z });
    return BSDFSample(wi, albedo.get(intr.uv) * Math::PiInv, pdf, BSDFType::Diffuse | BSDFType::Reflection);
}