#include "AABB.h"

AABB::AABB(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc)
{
    pMin = glm::min(glm::min(va, vb), vc);
    pMax = glm::max(glm::max(va, vb), vc);
}

AABB::AABB(const AABB &boundA, const AABB &boundB)
{
    pMin = glm::min(boundA.pMin, boundB.pMin);
    pMax = glm::max(boundA.pMax, boundB.pMax);
}

BoxHit AABB::hit(const Ray &ray)
{
    const float eps = 1e-6f;
    float tMin, tMax;
    glm::vec3 o = ray.ori;
    glm::vec3 d = ray.dir;

    glm::vec3 dInv = glm::vec3(1.0f) / d;

    if (glm::abs(d.x) > 1.0f - eps)
    {
        if (o.y > pMin.y && o.y < pMax.y && o.z > pMin.z && o.z < pMax.z)
        {
            tMin = (pMin.x - o.x) * dInv.x;
            tMax = (pMax.x - o.x) * dInv.x;
            if (tMin > tMax)
                std::swap(tMin, tMax);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
        else
            return {false};
    }

    if (glm::abs(d.y) > 1.0f - eps)
    {
        if (o.x > pMin.x && o.x < pMax.x && o.z > pMin.z && o.z < pMax.z)
        {
            tMin = (pMin.y - o.y) * dInv.y;
            tMax = (pMax.y - o.y) * dInv.y;
            if (tMin > tMax)
                std::swap(tMin, tMax);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
        else
            return {false};
    }

    if (glm::abs(d.z) > 1.0f - eps)
    {
        if (o.x > pMin.x && o.x < pMax.x && o.y > pMin.y && o.y < pMax.y)
        {
            tMin = (pMin.z - o.z) * dInv.z;
            tMax = (pMax.z - o.z) * dInv.z;
            if (tMin > tMax)
                std::swap(tMin, tMax);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
        else
            return {false};
    }

    glm::vec3 vtMin = (pMin - o) * dInv;
    glm::vec3 vtMax = (pMax - o) * dInv;

    if (vtMin.x > vtMax.x)
        std::swap(vtMin.x, vtMax.x);
    if (vtMin.y > vtMax.y)
        std::swap(vtMin.y, vtMax.y);
    if (vtMin.z > vtMax.z)
        std::swap(vtMin.z, vtMax.z);

    glm::vec3 dt = vtMax - vtMin;

    float tyz = vtMax.z - vtMin.y;
    float tzx = vtMax.x - vtMin.z;
    float txy = vtMax.y - vtMin.x;

    if (glm::abs(d.x) < eps)
    {
        if (dt.y + dt.z > tyz)
        {
            tMin = std::max(vtMin.y, vtMin.z);
            tMax = std::min(vtMax.y, vtMax.z);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
    }

    if (glm::abs(d.y) < eps)
    {
        if (dt.z + dt.x > tzx)
        {
            tMin = std::max(vtMin.z, vtMin.x);
            tMax = std::min(vtMax.z, vtMax.x);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
    }

    if (glm::abs(d.z) < eps)
    {
        if (dt.x + dt.y > txy)
        {
            tMin = std::max(vtMin.x, vtMin.y);
            tMax = std::min(vtMax.x, vtMax.y);
            return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
        }
    }

    if (dt.y + dt.z > tyz && dt.z + dt.x > tzx && dt.x + dt.y > txy)
    {
        tMin = std::max(std::max(vtMin.x, vtMin.y), vtMin.z);
        tMax = std::min(std::min(vtMax.x, vtMax.y), vtMax.z);
        return {tMax >= 0.0f && tMax >= tMin, tMin, tMax};
    }
    return {false};
}

float AABB::volume() const
{
    glm::vec3 vol = pMax - pMin;
    return vol.x * vol.y * vol.z;
}

glm::vec3 AABB::centroid() const
{
    return (pMin + pMax) * 0.5f;
}

float AABB::surfaceArea() const
{
    glm::vec3 vol = pMax - pMin;
    return 2.0f * (vol.x * vol.y + vol.y * vol.z + vol.z * vol.x);
}

int AABB::maxExtent() const
{
    return Math::maxExtent(pMax - pMin);
}

std::string AABB::toString()
{
    std::stringstream ss;
    ss << "[AABB pMin " << Math::vec3ToString(pMin) << ", pMax " << Math::vec3ToString(pMax) << " ]";
    return ss.str();
}