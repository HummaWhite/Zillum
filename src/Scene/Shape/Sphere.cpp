#include "Core/Shape.h"

std::optional<float> Sphere::closestHit(const Ray &ray) {
    Vec3f o = mTransform->getInversed(ray.ori);
    Vec3f d = mTransform->getInversed(ray.ori + ray.dir) - o;
    Vec3f c = center;

    float t = dot(d, c - o) / dot(d, d);
    float r = radius;

    float e = glm::length(o + d * t - c);
    if (e > r) {
        return std::nullopt;
    }

    float q = sqrt(glm::max(r * r - e * e, 0.0f));
    // 想不到吧，r * r - e * e还是可能小于0
    if (glm::length(o - c) < r) {
        if (!intersectFromInside)
            return std::nullopt;
        float res = t + q;
        return res >= 0 ? res : std::optional<float>();
    }
    float res = t - q;
    return res >= 0 ? res : std::optional<float>();
}

Vec3f Sphere::uniformSample(const Vec2f &u) {
    float t = Math::Pi * 2.0f * u.x;
    float p = Math::Pi * u.y;

    return Vec3f(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
}

Vec3f Sphere::normalGeom(const Vec3f &p) {
    return mTransform->getInversedNormal(glm::normalize(p - center));
}

float Sphere::surfaceArea() {
    return 4.0f * Math::Pi * radius * radius;
}

Vec2f Sphere::surfaceUV(const Vec3f &p) {
    auto oriP = mTransform->getInversed(p);
    return Transform::sphereToPlane(glm::normalize(oriP - center));
}