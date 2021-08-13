#include "Transform.h"

AABB Transform::getTransformedBox(const AABB &box)
{
    auto pMin = glm::vec3(matrix * glm::vec4(box.pMin, 1.0f));
    auto pMax = glm::vec3(matrix * glm::vec4(box.pMax, 1.0f));

    for (int i = 1; i < 7; i++)
    {
        float x = i & 0b001 ? box.pMax.x : box.pMin.x;
        float y = i & 0b010 ? box.pMax.y : box.pMin.y;
        float z = i & 0b100 ? box.pMax.z : box.pMin.z;

        auto p = glm::vec3(matrix * glm::vec4(x, y, z, 1.0f));
        pMin = glm::min(pMin, p);
        pMax = glm::max(pMax, p);
    }

    return AABB(pMin, pMax);
}

glm::vec2 Transform::sphereToPlane(const glm::vec3 &uv)
{
    float theta = glm::atan(uv.y, uv.x);
    if (theta < 0.0f)
        theta += Math::Pi * 2.0f;

    float phi = glm::atan(glm::length(glm::vec2(uv)), uv.z);
    if (phi < 0.0f)
        phi += Math::Pi * 2.0f;

    return {theta / (Math::Pi * 2.0f), phi * Math::PiInv};
}

glm::vec3 Transform::planeToSphere(const glm::vec2 &uv)
{
    float theta = uv.x * Math::Pi * 2.0f;
    float phi = uv.y * Math::Pi;
    return {cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi)};
}

glm::vec2 Transform::toConcentricDisk(const glm::vec2 &uv)
{
    if (uv.x == 0.0f && uv.y == 0.0f)
        return glm::vec2(0.0f);
    glm::vec2 v = uv * 2.0f - 1.0f;

    float phi, r;
    if (v.x * v.x > v.y * v.y)
    {
        r = v.x;
        phi = Math::Pi * v.y / v.x * 0.25f;
    }
    else
    {
        r = v.y;
        phi = Math::Pi * 0.5f - Math::Pi * v.x / v.y * 0.25f;
    }
    return glm::vec2(r * glm::cos(phi), r * glm::sin(phi));
}

glm::vec3 Transform::normalToWorld(const glm::vec3 &N, const glm::vec3 &dir)
{
    return glm::normalize(Math::TBNMatrix(N) * dir);
}

namespace Math
{
    std::pair<glm::vec3, float> sampleHemisphereCosine(const glm::vec3 &N, const glm::vec2 &u)
    {
        glm::vec2 uv = Transform::toConcentricDisk(u);
        float z = glm::sqrt(1.0f - glm::dot(uv, uv));
        glm::vec3 v = Transform::normalToWorld(N, glm::vec3(uv, z));
        return {v, PiInv * z};
    }
}