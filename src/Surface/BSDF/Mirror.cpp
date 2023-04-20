#include "Core/BSDF.h"

Spectrum MirrorBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    return Spectrum(0.0f);
}

float MirrorBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    return 0.0f;
}

std::optional<BSDFSample> MirrorBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    Vec3f wi = { -wo.x, -wo.y, wo.z };
    return BSDFSample(wi, baseColor.get(uv), 1.0f, BSDFType::Delta | BSDFType::Reflection);
}