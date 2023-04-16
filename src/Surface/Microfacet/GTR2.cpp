#include "Core/Microfacet.h"

GTR2Distrib::GTR2Distrib(float roughness, bool sampleVisible, float aniso)
{
    float r2 = roughness * roughness;
    visible = sampleVisible;
    //float al = glm::sqrt(1.0f - aniso * 0.9f);
    //alpha = Vec2f(r2 / al, r2 * al);
    alpha = r2;
}

float GTR2Distrib::d(const Vec3f &m) const
{
    return ggx(m.z, alpha);
}

float GTR2Distrib::pdf(const Vec3f &m, const Vec3f &wo) const
{
    if (!visible)
        return d(m);
    return d(m) * schlickG(wo.z, alpha) * Math::absDot(m, wo) / glm::abs(wo.z);
}

Vec3f GTR2Distrib::sampleWm(const Vec3f &wo, const Vec2f &u) const
{
    if (visible)
    {
        Vec3f vh = glm::normalize(wo * Vec3f(alpha, alpha, 1.0f));

        float lensq = vh.x * vh.x + vh.y * vh.y;
        Vec3f t1 = lensq > 0.0f ? Vec3f(-vh.y, vh.x, 0.0f) / glm::sqrt(lensq) : Vec3f(1.0f, 0.0f, 0.0f);
        Vec3f t2 = glm::cross(vh, t1);

        Vec2f xi = Transform::toConcentricDisk(u);
        float s = 0.5f * (1.0f + vh.z);
        xi.y = (1.0f - s) * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x)) + s * xi.y;

        Vec3f h = t1 * xi.x + t2 * xi.y + vh * glm::sqrt(glm::max(0.0f, 1.0f - glm::dot(xi, xi)));
        return glm::normalize(Vec3f(h.x * alpha, h.y * alpha, glm::max(0.0f, h.z)));
    }
    else
    {
        Vec2f xi = Transform::toConcentricDisk(u);

        Vec3f h = Vec3f(xi.x, xi.y, glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y)));
        return glm::normalize(h * Vec3f(alpha, alpha, 1.0f));
    }
}

float GTR2Distrib::g(const Vec3f &wo, const Vec3f &wi) const
{
    return smithG(wo, wi, alpha);
}