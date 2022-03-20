#include "../../include/Core/Material.h"

Spectrum Lambertian::bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    return albedo * Math::PiInv;
}

float Lambertian::pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode)
{
    return glm::dot(n, wi) * Math::PiInv;
}

std::optional<BSDFSample> Lambertian::sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode)
{
    auto [wi, pdf] = Math::sampleHemisphereCosine(n, { u.y, u.z });
    return BSDFSample(wi, pdf, BXDF::Diffuse, albedo * Math::PiInv);
}