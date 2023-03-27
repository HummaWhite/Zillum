#include "Core/BSDF.h"

Spectrum Lambertian::bsdf(const SurfaceIntr &intr, TransportMode mode) {
    return albedo.get(intr.uv) * Math::PiInv;
}

float Lambertian::pdf(const SurfaceIntr &intr, TransportMode mode) {
    return glm::dot(intr.n, intr.wi) * Math::PiInv;
}

std::optional<BSDFSample> Lambertian::sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode) {
    auto [wi, pdf] = Math::sampleHemisphereCosine(intr.n, { u.y, u.z });
    return BSDFSample(wi, albedo.get(intr.uv) * Math::PiInv, pdf, BSDFType::Diffuse | BSDFType::Reflection);
}