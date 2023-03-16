#include "Core/Shape.h"

std::optional<float> Quad::closestHit(const Ray &ray) {
    Vec3f o = mTransform.getInversed(ray.ori);
    Vec3f d = mTransform.getInversed(ray.ori + ray.dir) - o;

    Ray inversedRay = {o, d};
    Vec3f vd = vb + vc - va;

    auto ha = Triangle(va, vb, vc).closestHit(inversedRay);
    auto hb = Triangle(vc, vb, vd).closestHit(inversedRay);

    if (ha.has_value()) {
        return ha;
    }
    if (hb.has_value()) {
        return hb;
    }

    return std::nullopt;
}

Vec3f Quad::uniformSample(const Vec2f &u) {
    return mTransform.get((vb - va) * u.x + (vc - va) * u.y + va);
}

Vec3f Quad::normalGeom(const Vec3f &p) {
    Vec3f N = glm::normalize(glm::cross(vb - va, vc - va));
    return glm::normalize(mTransform.getInversedNormal(N));
}

float Quad::surfaceArea() {
    return glm::length(glm::cross(vc - va, vb - va));
}

AABB Quad::bound() {
    Vec3f pa = mTransform.get(va);
    Vec3f pb = mTransform.get(vb);
    Vec3f pc = mTransform.get(vc);
    Vec3f pd = pb + pc - pa;
    return AABB(AABB(pa, pb, pc), AABB(pb, pc, pd));
}