#include "../../include/Core/Material.h"

Vec3f ThinDielectric::bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode)
{
    return Vec3f(0.0f);
}

SampleWithBsdf ThinDielectric::sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode)
{
    float refl = fresnelDielectric(glm::dot(N, Wo), ior);
    float trans = 1.0f - refl;
    if (refl < 1.0f)
    {
        refl += trans * trans * refl / (1.0f - refl * refl);
        trans = 1.0f - refl;
    }

    if (u1 < refl)
    {
        auto Wi = glm::reflect(-Wo, N);
        return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
    }
    else
    {
        return SampleWithBsdf(Sample(-Wo, 1.0f, BXDF::SpecTrans), tint);
    }
}