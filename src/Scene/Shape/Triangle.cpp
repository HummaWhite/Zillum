#include "../../../include/Core/Shape.h"

std::optional<float> Triangle::closestHit(const Ray &ray)
{
    const float eps = 1e-6;

    Vec3f ab = vb - va;
    Vec3f ac = vc - va;

    Vec3f o = mTransform->getInversed(ray.ori);
    Vec3f d = mTransform->getInversed(ray.ori + ray.dir) - o;

    Vec3f p = glm::cross(d, ac);

    float det = glm::dot(ab, p);

    if (glm::abs(det) < eps)
        return std::nullopt;

    Vec3f ao = o - va;
    if (det < 0.0f)
    {
        ao = -ao;
        det = -det;
    }

    float u = glm::dot(ao, p);
    if (u < 0.0f || u > det)
        return std::nullopt;

    Vec3f q = glm::cross(ao, ab);

    float v = glm::dot(d, q);
    if (v < 0.0f || u + v > det)
        return std::nullopt;

    float t = glm::dot(ac, q) / det;
    return t > 0.0f ? t : std::optional<float>();
}

Vec3f Triangle::uniformSample(const Vec2f &u)
{
    float r = glm::sqrt(u.y);
    float a = 1.0f - r;
    float b = u.x * r;

    Vec3f p = va * (1.0f - a - b) + vb * a + vc * b;
    return mTransform->get(p);
}

Vec3f Triangle::normalGeom(const Vec3f &p)
{
    Vec3f N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(mTransform->getInversedNormal(N));
}

float Triangle::surfaceArea()
{
    return 0.5f * glm::length(glm::cross(vc - va, vb - va));
}

Vec2f Triangle::surfaceUV(const Vec3f &p)
{
    Vec3f oriP = mTransform->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    return Vec2f(u, v);
}

AABB Triangle::bound()
{
    return AABB(mTransform->get(va), mTransform->get(vb), mTransform->get(vc));
}