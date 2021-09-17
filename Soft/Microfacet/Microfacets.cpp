#include "Microfacets.h"

GGXDistrib::GGXDistrib(float roughness, bool sampleVisible, float aniso)
{
    float r2 = roughness * roughness;
    visible = sampleVisible;
    //float al = glm::sqrt(1.0f - aniso * 0.9f);
    //alpha = Vec2f(r2 / al, r2 * al);
    alpha = r2;
}

float GGXDistrib::d(const Vec3f &N, const Vec3f &M)
{
    return Microfacet::ggx(glm::dot(N, M), alpha);
}

float GGXDistrib::pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo)
{
    if (!visible)
        return d(N, M);
    return d(N, M) * Microfacet::schlickG(glm::dot(N, Wo), alpha) * Math::absDot(M, Wo) / Math::absDot(N, Wo);
}

Vec3f GGXDistrib::sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u)
{
    if (visible)
    {
        glm::mat3 TBN = Math::TBNMatrix(N);
        glm::mat3 TBNinv = glm::inverse(TBN);

        Vec3f Vh = glm::normalize((TBNinv * Wo) * Vec3f(alpha, alpha, 1.0f));

        float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
        Vec3f T1 = lensq > 0.0f ? Vec3f(-Vh.y, Vh.x, 0.0f) / glm::sqrt(lensq) : Vec3f(1.0f, 0.0f, 0.0f);
        Vec3f T2 = glm::cross(Vh, T1);

        Vec2f xi = Transform::toConcentricDisk(u);
        float s = 0.5f * (1.0f + Vh.z);
        xi.y = (1.0f - s) * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x)) + s * xi.y;

        Vec3f H = T1 * xi.x + T2 * xi.y + Vh * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y));
        H = glm::normalize(Vec3f(H.x * alpha, H.y * alpha, glm::max(0.0f, H.z)));
        return glm::normalize(TBN * H);
    }
    else
    {
        Vec2f xi = Transform::toConcentricDisk(u);

        Vec3f H = Vec3f(xi.x, xi.y, glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y)));
        H = glm::normalize(H * Vec3f(alpha, alpha, 1.0f));
        return glm::normalize(Transform::normalToWorld(N, H));
    }
}

float GGXDistrib::g(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi)
{
    return Microfacet::smithG(N, Wo, Wi, alpha);
}

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