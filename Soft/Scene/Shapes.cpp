#include "Shapes.h"

std::optional<float> Sphere::closestHit(const Ray &ray)
{
    glm::vec3 o = transform->getInversed(ray.ori);
    glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;
    glm::vec3 c = center;

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

glm::vec3 Sphere::uniformSample(const glm::vec2 &u)
{
    float t = Math::Pi * 2.0f * u.x;
    float p = Math::Pi * u.y;

    return glm::vec3(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
}

glm::vec3 Sphere::surfaceNormal(const glm::vec3 &p)
{
    return transform->getInversedNormal(glm::normalize(p - center));
}

float Sphere::surfaceArea()
{
    return 4.0f * Math::Pi * radius * radius;
}

glm::vec2 Sphere::surfaceUV(const glm::vec3 &p)
{
    auto oriP = transform->getInversed(p);
    return Transform::sphereToPlane(glm::normalize(oriP - center));
}

std::optional<float> Triangle::closestHit(const Ray &ray)
{
    const float eps = 1e-6;

    glm::vec3 ab = vb - va;
    glm::vec3 ac = vc - va;

    glm::vec3 o = transform->getInversed(ray.ori);
    glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

    glm::vec3 p = glm::cross(d, ac);

    float det = glm::dot(ab, p);

    if (glm::abs(det) < eps)
        return std::nullopt;

    glm::vec3 ao = o - va;
    if (det < 0.0f)
    {
        ao = -ao;
        det = -det;
    }

    float u = glm::dot(ao, p);
    if (u < 0.0f || u > det)
        return std::nullopt;

    glm::vec3 q = glm::cross(ao, ab);

    float v = glm::dot(d, q);
    if (v < 0.0f || u + v > det)
        return std::nullopt;

    float t = glm::dot(ac, q) / det;
    return t > 0.0f ? t : std::optional<float>();
}

glm::vec3 Triangle::uniformSample(const glm::vec2 &u)
{
    float r = glm::sqrt(u.y);
    float a = 1.0f - r;
    float b = u.x * r;

    glm::vec3 p = va * (1.0f - a - b) + vb * a + vc * b;
    return transform->get(p);
}

glm::vec3 Triangle::surfaceNormal(const glm::vec3 &p)
{
    glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(transform->getInversedNormal(N));
}

float Triangle::surfaceArea()
{
    return 0.5f * glm::length(glm::cross(vc - va, vb - va));
}

glm::vec2 Triangle::surfaceUV(const glm::vec3 &p)
{
    glm::vec3 oriP = transform->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    return glm::vec2(u, v);
}

AABB Triangle::bound()
{
    return AABB(transform->get(va), transform->get(vb), transform->get(vc));
}

glm::vec3 MeshTriangle::surfaceNormal(const glm::vec3 &p)
{
    auto [va, vb, vc] = triangle.vertices();
    glm::vec3 oriP = triangle.getTransform()->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float la = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float lb = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    float lc = glm::length(glm::cross(va - oriP, vb - oriP)) * areaInv;

    return glm::normalize(triangle.getTransform()->getInversedNormal(na * la + nb * lb + nc * lc));
}

glm::vec2 MeshTriangle::surfaceUV(const glm::vec3 &p)
{
    auto [va, vb, vc] = triangle.vertices();
    glm::vec3 oriP = triangle.getTransform()->getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;

    return glm::vec2(u, v);
}

void MeshTriangle::setTransform(TransformPtr trans)
{
    transform = trans;
    triangle.setTransform(trans);
}

std::optional<float> Quad::closestHit(const Ray &ray)
{
    glm::vec3 o = transform->getInversed(ray.ori);
    glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

    Ray inversedRay = {o, d};
    glm::vec3 vd = vb + vc - va;

    auto ha = Triangle(va, vb, vc).closestHit(inversedRay);
    auto hb = Triangle(vc, vb, vd).closestHit(inversedRay);

    if (ha.has_value())
        return ha;
    if (hb.has_value())
        return hb;

    return std::nullopt;
}

glm::vec3 Quad::uniformSample(const glm::vec2 &u)
{
    return transform->get((vb - va) * u.x + (vc - va) * u.y + va);
}

glm::vec3 Quad::surfaceNormal(const glm::vec3 &p)
{
    glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(transform->getInversedNormal(N));
}

float Quad::surfaceArea()
{
    return glm::length(glm::cross(vc - va, vb - va));
}

AABB Quad::bound()
{
    glm::vec3 pa = transform->get(va);
    glm::vec3 pb = transform->get(vb);
    glm::vec3 pc = transform->get(vc);
    glm::vec3 pd = pb + pc - pa;
    return AABB(AABB(pa, pb, pc), AABB(pb, pc, pd));
}