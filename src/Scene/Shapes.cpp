#include "Shapes.h"

std::optional<float> Sphere::closestHit(const Ray &ray)
{
    Vec3f o = transform->getInversed(ray.ori);
    Vec3f d = transform->getInversed(ray.ori + ray.dir) - o;
    Vec3f c = center;

    float t = dot(d, c - o) / dot(d, d);
    float r = radius;

    float e = glm::length(o + d * t - c);
    if (e > r)
        return std::nullopt;

    float q = sqrt(glm::max(r * r - e * e, 0.0f));
    // 想不到吧，r * r - e * e还是可能小于0
    if (glm::length(o - c) < r)
    {
        if (!intersectFromInside)
            return std::nullopt;
        float res = t + q;
        return res >= 0 ? res : std::optional<float>();
    }
    float res = t - q;
    return res >= 0 ? res : std::optional<float>();
}

Vec3f Sphere::uniformSample(const Vec2f &u)
{
    float t = Math::Pi * 2.0f * u.x;
    float p = Math::Pi * u.y;

    return Vec3f(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
}

Vec3f Sphere::normalGeom(const Vec3f &p)
{
    return transform->getInversedNormal(glm::normalize(p - center));
}

float Sphere::surfaceArea()
{
    return 4.0f * Math::Pi * radius * radius;
}

Vec2f Sphere::surfaceUV(const Vec3f &p)
{
    auto oriP = transform->getInversed(p);
    return Transform::sphereToPlane(glm::normalize(oriP - center));
}

std::optional<float> Triangle::closestHit(const Ray &ray)
{
    const float eps = 1e-6;

    Vec3f ab = vb - va;
    Vec3f ac = vc - va;

    Vec3f o = transform->getInversed(ray.ori);
    Vec3f d = transform->getInversed(ray.ori + ray.dir) - o;

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
    return transform->get(p);
}

Vec3f Triangle::normalGeom(const Vec3f &p)
{
    Vec3f N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(transform->getInversedNormal(N));
}

float Triangle::surfaceArea()
{
    return 0.5f * glm::length(glm::cross(vc - va, vb - va));
}

Vec2f Triangle::surfaceUV(const Vec3f &p)
{
    Vec3f oriP = transform->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    return Vec2f(u, v);
}

AABB Triangle::bound()
{
    return AABB(transform->get(va), transform->get(vb), transform->get(vc));
}

Vec3f MeshTriangle::normalShading(const Vec3f &p)
{
    auto [va, vb, vc] = triangle.vertices();
    Vec3f oriP = triangle.getTransform()->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float la = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float lb = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    float lc = glm::length(glm::cross(va - oriP, vb - oriP)) * areaInv;

    return glm::normalize(triangle.getTransform()->getInversedNormal(na * la + nb * lb + nc * lc));
}

Vec2f MeshTriangle::surfaceUV(const Vec3f &p)
{
    auto [va, vb, vc] = triangle.vertices();
    Vec3f oriP = triangle.getTransform()->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;

    return Vec2f(u, v);
}

void MeshTriangle::setTransform(TransformPtr trans)
{
    transform = trans;
    triangle.setTransform(trans);
}

std::optional<float> Quad::closestHit(const Ray &ray)
{
    Vec3f o = transform->getInversed(ray.ori);
    Vec3f d = transform->getInversed(ray.ori + ray.dir) - o;

    Ray inversedRay = {o, d};
    Vec3f vd = vb + vc - va;

    auto ha = Triangle(va, vb, vc).closestHit(inversedRay);
    auto hb = Triangle(vc, vb, vd).closestHit(inversedRay);

    if (ha.has_value())
        return ha;
    if (hb.has_value())
        return hb;

    return std::nullopt;
}

Vec3f Quad::uniformSample(const Vec2f &u)
{
    return transform->get((vb - va) * u.x + (vc - va) * u.y + va);
}

Vec3f Quad::normalGeom(const Vec3f &p)
{
    Vec3f N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(transform->getInversedNormal(N));
}

float Quad::surfaceArea()
{
    return glm::length(glm::cross(vc - va, vb - va));
}

AABB Quad::bound()
{
    Vec3f pa = transform->get(va);
    Vec3f pb = transform->get(vb);
    Vec3f pc = transform->get(vc);
    Vec3f pd = pb + pc - pa;
    return AABB(AABB(pa, pb, pc), AABB(pb, pc, pd));
}