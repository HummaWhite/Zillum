#include "Core/Microfacet.h"

float GTR1Distrib::d(const Vec3f &m) const
{
    return gtr1(glm::abs(m.z), alpha);
}

float GTR1Distrib::pdf(const Vec3f &m, const Vec3f &wo) const
{
    return d(m) * glm::abs(m.z);
}

Vec3f GTR1Distrib::sampleWm(const Vec3f &wo, const Vec2f &u) const
{
    float cosTheta = glm::sqrt(glm::max(0.0f, (1.0f - glm::pow(alpha, 1.0f - u.x)) / (1.0f - alpha)));
    float sinTheta = glm::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = 2.0f * u.y * Math::Pi;

    Vec3f m = glm::normalize(Vec3f(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta));
    if (!Math::sameHemisphere(wo, m))
        m = -m;
    return m;
}