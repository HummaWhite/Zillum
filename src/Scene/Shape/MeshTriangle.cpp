#include "Core/Shape.h"

Vec3f MeshTriangle::normalShading(const Vec3f &p) {
    auto [va, vb, vc] = triangle.vertices();
    Vec3f oriP = triangle.getTransform().getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float la = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float lb = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    float lc = glm::length(glm::cross(va - oriP, vb - oriP)) * areaInv;

    return glm::normalize(triangle.getTransform().getInversedNormal(na * la + nb * lb + nc * lc));
}

Vec2f MeshTriangle::surfaceUV(const Vec3f &p) {
    auto [va, vb, vc] = triangle.vertices();
    Vec3f oriP = triangle.getTransform().getInversed(p);

    float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
    float la = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
    float lb = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
    float lc = glm::length(glm::cross(va - oriP, vb - oriP)) * areaInv;

    return ta * la + tb * lb + tc * lc;
}

void MeshTriangle::setTransform(const Transform& trans) {
    mTransform = trans;
    triangle.setTransform(trans);
}