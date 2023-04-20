#include "Core/BSDF.h"

Spectrum LambertBSDF::bsdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    return albedo.get(uv) * Math::PiInv;
}

float LambertBSDF::pdf(Vec3f wo, Vec3f wi, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    return wi.z * Math::PiInv;
}

std::optional<BSDFSample> LambertBSDF::sample(Vec3f wo, Vec2f uv, TransportMode mode, Sampler* sampler) const {
    Vec3f wi = Math::sampleHemisphereCosine(sampler->get2());
    return BSDFSample(wi, albedo.get(uv) * Math::PiInv, wi.z * Math::PiInv, BSDFType::Diffuse | BSDFType::Reflection);
}