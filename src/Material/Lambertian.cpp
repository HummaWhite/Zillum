#include "Material.h"

Vec3f Lambertian::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return albedo * Math::PiInv;
}

float Lambertian::pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return glm::dot(Wi, N) * Math::PiInv;
}

Sample Lambertian::getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    auto [Wi, pdf] = Math::sampleHemisphereCosine(N, u2);
    return Sample(Wi, pdf, BXDF::Diffuse);
}