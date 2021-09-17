#include "Light.h"

std::optional<LightLiSample> Light::sampleLi(Vec3f x, Vec2f u)
{
    Vec3f y = uniformSample(u);
    Vec3f Wi = glm::normalize(y - x);
    Vec3f N = surfaceNormal(y);
    float cosTheta = glm::dot(N, -Wi);

    if (cosTheta <= 1e-6f)
        return std::nullopt;

    float dist = glm::distance(x, y);
    float pdf = dist * dist / (surfaceArea() * cosTheta);

    return LightLiSample{Wi, Le({y, -Wi}), dist, pdf};
}

float Light::pdfLi(const Vec3f &x, const Vec3f &y)
{
    auto N = surfaceNormal(y);
    auto Wi = glm::normalize(y - x);
    float cosTheta = Math::satDot(N, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;

    return Math::distSquare(x, y) / (surfaceArea() * cosTheta);
}

Vec3f Light::Le(Ray ray)
{
    if (glm::dot(surfaceNormal(ray.ori), ray.dir) <= 0.0f)
        return Vec3f(0.0f);
    return power / (2.0f * Math::Pi * surfaceArea());
}

LightLeSample Light::sampleLe(const std::array<float, 6> &u)
{
    auto ori = uniformSample({ u[0], u[1] });
    float pdfPos = 1.0f / surfaceArea();

    auto N = surfaceNormal(ori);
    auto [We, pdfDir] = Math::sampleHemisphereCosine(N, { u[2], u[3] });
    Ray ray(ori, We);

    return { ray, Le(ray), pdfPos, pdfDir };
}

Ray Light::getRandomRay()
{
    Vec3f ori = uniformSample({});
    Vec3f N = surfaceNormal(ori);
    Vec3f dir = Transform::normalToWorld(N, {});
    return {ori + dir * 1e-4f, dir};
}