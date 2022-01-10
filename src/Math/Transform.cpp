#include "../../include/Core/Transform.h"

AABB Transform::getTransformedBox(const AABB &box)
{
    auto pMin = Vec3f(matrix * Vec4f(box.pMin, 1.0f));
    auto pMax = Vec3f(matrix * Vec4f(box.pMax, 1.0f));

    for (int i = 1; i < 7; i++)
    {
        float x = i & 0b001 ? box.pMax.x : box.pMin.x;
        float y = i & 0b010 ? box.pMax.y : box.pMin.y;
        float z = i & 0b100 ? box.pMax.z : box.pMin.z;

        auto p = Vec3f(matrix * Vec4f(x, y, z, 1.0f));
        pMin = glm::min(pMin, p);
        pMax = glm::max(pMax, p);
    }

    return AABB(pMin, pMax);
}

Vec2f Transform::sphereToPlane(const Vec3f &uv)
{
    float theta = glm::atan(uv.y, uv.x);
    if (theta < 0.0f)
        theta += Math::Pi * 2.0f;

    float phi = glm::atan(glm::length(Vec2f(uv)), uv.z);
    if (phi < 0.0f)
        phi += Math::Pi * 2.0f;

    return {theta / (Math::Pi * 2.0f), phi * Math::PiInv};
}

Vec3f Transform::planeToSphere(const Vec2f &uv)
{
    float theta = uv.x * Math::Pi * 2.0f;
    float phi = uv.y * Math::Pi;
    return {cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi)};
}

Vec2f Transform::toConcentricDisk(const Vec2f &uv)
{
    if (uv.x == 0.0f && uv.y == 0.0f)
        return Vec2f(0.0f);
    Vec2f v = uv * 2.0f - 1.0f;

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
    return Vec2f(r * glm::cos(phi), r * glm::sin(phi));
}

Vec3f Transform::normalToWorld(const Vec3f &N, const Vec3f &dir)
{
    return glm::normalize(Math::TBNMatrix(N) * dir);
}

namespace Math
{
    std::pair<Vec3f, float> sampleHemisphereCosine(const Vec3f &N, const Vec2f &u)
    {
        Vec2f uv = Transform::toConcentricDisk(u);
        float z = glm::sqrt(1.0f - glm::dot(uv, uv));
        Vec3f v = Transform::normalToWorld(N, Vec3f(uv, z));
        return {v, PiInv * z};
    }
}