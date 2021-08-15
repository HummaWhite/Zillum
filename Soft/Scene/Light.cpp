#include "Light.h"

glm::vec3 Light::Le(const glm::vec3 &y, const glm::vec3 &We)
{
    if (glm::dot(surfaceNormal(y), We) <= 0.0f)
        return glm::vec3(0.0f);
    return power / (2.0f * Math::Pi * surfaceArea());
}

std::optional<LightLiSample> Light::sampleLi(glm::vec3 x, glm::vec2 u)
{
    glm::vec3 y = uniformSample(u);
    glm::vec3 Wi = glm::normalize(y - x);
    glm::vec3 N = surfaceNormal(y);
    float cosTheta = glm::dot(N, -Wi);

    if (cosTheta <= 1e-6f)
        return std::nullopt;

    float dist = glm::distance(x, y);
    float pdf = dist * dist / (surfaceArea() * cosTheta);
    glm::vec3 weight = Le(y, -Wi);

    return LightLiSample{Wi, weight, dist, pdf};
}

float Light::pdfLi(const glm::vec3 &x, const glm::vec3 &y)
{
    auto N = surfaceNormal(y);
    auto Wi = glm::normalize(y - x);
    float cosTheta = Math::satDot(N, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;

    return Math::distSquare(x, y) / (surfaceArea() * cosTheta);
}

LightLeSample Light::sampleLe(const std::array<float, 6> &u)
{
    auto ori = uniformSample({ u[0], u[1] });
    float pdfPos = 1.0f / surfaceArea();

    auto N = surfaceNormal(ori);
    auto We = Transform::planeToSphere({ u[2], u[3] });
    We = Transform::normalToWorld(N, We);

    return { { ori, We }, Le(ori, We), pdfPos * Math::PiInv * 0.5f };
}

Ray Light::getRandomRay()
{
    glm::vec3 ori = uniformSample({});
    glm::vec3 N = surfaceNormal(ori);
    glm::vec3 dir = Transform::normalToWorld(N, {});
    return {ori + dir * 1e-4f, dir};
}