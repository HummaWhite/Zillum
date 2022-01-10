#include "../../include/Core/Microfacet.h"

float GTR1Distrib::d(const Vec3f &N, const Vec3f &M)
{
    return Microfacet::gtr1(Math::absDot(N, M), alpha);
}

float GTR1Distrib::pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo)
{
    return d(N, M) * Math::absDot(N, M);
}

Vec3f GTR1Distrib::sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u)
{
    float cosTheta = glm::sqrt(glm::max(0.0f, (1.0f - glm::pow(alpha, 1.0f - u.x)) / (1.0f - alpha)));
    float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * u.y * Math::Pi;

    Vec3f M = glm::normalize(Vec3f(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta));
    if (!Math::sameHemisphere(N, Wo, M))
        M = -M;

    return glm::normalize(Transform::normalToWorld(N, M));
}