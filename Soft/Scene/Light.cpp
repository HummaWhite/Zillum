#include "Light.h"

std::optional<LightLiSample> Light::sampleLi(Vec3f ref, Vec2f u)
{
    Vec3f y = uniformSample(u);
    Vec3f Wi = glm::normalize(y - ref);
    Vec3f N = normalGeom(y);
    float cosTheta = glm::dot(N, -Wi);

    if (cosTheta <= 1e-6f)
        return std::nullopt;

    float dist = glm::distance(ref, y);
    float pdf = dist * dist / (surfaceArea() * cosTheta);

    return LightLiSample{Wi, Le({y, -Wi}), dist, pdf};
}

float Light::pdfLi(const Vec3f &ref, const Vec3f &y)
{
    auto N = normalGeom(y);
    auto Wi = glm::normalize(y - ref);
    float cosTheta = Math::satDot(N, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;

    return Math::distSquare(ref, y) / (surfaceArea() * cosTheta);
}

Vec3f Light::Le(Ray ray)
{
    if (glm::dot(normalGeom(ray.ori), ray.dir) <= 0.0f)
        return Vec3f(0.0f);
    return power / (2.0f * Math::Pi * surfaceArea());
}

LightLeSample Light::sampleLe(const std::array<float, 6> &u)
{
    auto ori = uniformSample({ u[0], u[1] });
    float pdfPos = 1.0f / surfaceArea();

    auto N = normalGeom(ori);
    auto [We, pdfDir] = Math::sampleHemisphereCosine(N, { u[2], u[3] });
    Ray ray(ori, We);

    return { ray, Le(ray), pdfPos, pdfDir };
}

Ray Light::getRandomRay()
{
    Vec3f ori = uniformSample({});
    Vec3f N = normalGeom(ori);
    Vec3f dir = Transform::normalToWorld(N, {});
    return {ori + dir * 1e-4f, dir};
}