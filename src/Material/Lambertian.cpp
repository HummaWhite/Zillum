#include "../../include/Core/Material.h"

Vec3f Lambertian::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return albedo * Math::PiInv;
}

float Lambertian::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return glm::dot(Wi, N) * Math::PiInv;
}

std::optional<BSDFSample> Lambertian::sample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto [Wi, pdf] = Math::sampleHemisphereCosine(N, u2);
    return BSDFSample(Wi, pdf, BXDF::Diffuse, albedo * Math::PiInv);
}