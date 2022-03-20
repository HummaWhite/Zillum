#include "../../include/Core/Microfacet.h"

float GTR1Distrib::d(const Vec3f &n, const Vec3f &m)
{
    return gtr1(Math::absDot(n, m), alpha);
}

float GTR1Distrib::pdf(const Vec3f &n, const Vec3f &m, const Vec3f &wo)
{
    return d(n, m) * Math::absDot(n, m);
}

Vec3f GTR1Distrib::sampleWm(const Vec3f &n, const Vec3f &wo, const Vec2f &u)
{
    float cosTheta = glm::sqrt(glm::max(0.0f, (1.0f - glm::pow(alpha, 1.0f - u.x)) / (1.0f - alpha)));
    float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * u.y * Math::Pi;

    Vec3f m = glm::normalize(Vec3f(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta));
    if (!Math::sameHemisphere(n, wo, m))
        m = -m;

    return glm::normalize(Transform::normalToWorld(n, m));
}