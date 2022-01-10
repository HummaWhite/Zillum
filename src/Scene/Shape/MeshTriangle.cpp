#include "Shape.h"

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