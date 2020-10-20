#ifndef AABB_H
#define AABB_H

#include <iostream>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Math.h"
#include "../Ray.h"

struct AABB
{
	AABB() {}

	AABB(const glm::vec3 _pMin, const glm::vec3 _pMax):
		pMin(_pMin), pMax(_pMax) {}

	AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax):
		pMin(xMin, yMin, zMin), pMax(xMax, yMax, zMax) {}

	AABB(const glm::vec3 &center, float radius):
		pMin(center - glm::vec3(radius)), pMax(center + glm::vec3(radius)) {}

	AABB(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc)
	{
		pMin = glm::min(glm::min(va, vb), vc);
		pMax = glm::max(glm::max(va, vb), vc);
	}

	AABB(const AABB &boundA, const AABB &boundB)
	{
		pMin = glm::min(boundA.pMin, boundB.pMin);
		pMax = glm::max(boundA.pMax, boundB.pMax);
	}

	inline bool hit(const Ray &ray, float &tMin, float &tMax)
	{
		const float eps = 0.000001f;
		glm::vec3 o = ray.ori;
		glm::vec3 d = ray.dir;

		float dxInv = 1.0f / d.x;
		float dyInv = 1.0f / d.y;
		float dzInv = 1.0f / d.z;

		if (glm::abs(d.x) > 1.0f - eps)
		{
			if (o.y > pMin.y && o.y < pMax.y && o.z > pMin.z && o.z < pMax.z)
			{
				tMin = (pMin.x - o.x) * dxInv;
				tMax = (pMax.x - o.x) * dxInv;
				if (tMin > tMax) std::swap(tMin, tMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
			else return false;
		}

		if (glm::abs(d.y) > 1.0f - eps)
		{
			if (o.x > pMin.x && o.x < pMax.x && o.z > pMin.z && o.z < pMax.z)
			{
				tMin = (pMin.y - o.y) * dyInv;
				tMax = (pMax.y - o.y) * dyInv;
				if (tMin > tMax) std::swap(tMin, tMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
			else return false;
		}

		if (glm::abs(d.z) > 1.0f - eps)
		{
			if (o.x > pMin.x && o.x < pMax.x && o.y > pMin.y && o.y < pMax.y)
			{
				tMin = (pMin.z - o.z) * dzInv;
				tMax = (pMax.z - o.z) * dzInv;
				if (tMin > tMax) std::swap(tMin, tMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
			else return false;
		}

		float txMin = (pMin.x - o.x) * dxInv, txMax = (pMax.x - o.x) * dxInv;
		float tyMin = (pMin.y - o.y) * dyInv, tyMax = (pMax.y - o.y) * dyInv;
		float tzMin = (pMin.z - o.z) * dzInv, tzMax = (pMax.z - o.z) * dzInv;

		if (txMin > txMax) std::swap(txMin, txMax);
		if (tyMin > tyMax) std::swap(tyMin, tyMax);
		if (tzMin > tzMax) std::swap(tzMin, tzMax);

		float dtx = txMax - txMin;
		float dty = tyMax - tyMin;
		float dtz = tzMax - tzMin;

		float tyz = tzMax - tyMin;
		float tzx = txMax - tzMin;
		float txy = tyMax - txMin;

		if (glm::abs(d.x) < eps)
		{
			if (dty + dtz > tyz)
			{
				tMin = std::max(tyMin, tzMin);
				tMax = std::min(tyMax, tzMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
		}

		if (glm::abs(d.y) < eps)
		{
			if (dtz + dtx > tzx)
			{
				tMin = std::max(tzMin, txMin);
				tMax = std::min(tzMax, txMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
		}

		if (glm::abs(d.z) < eps)
		{
			if (dtx + dty > txy)
			{
				tMin = std::max(txMin, tyMin);
				tMax = std::min(txMax, tyMax);
				return tMax >= 0.0f && tMax >= tMin;
			}
		}

		if (dty + dtz > tyz && dtz + dtx > tzx && dtx + dty > txy)
		{
			tMin = std::max(std::max(txMin, tyMin), tzMin);
			tMax = std::min(std::min(txMax, tyMax), tzMax);
			return tMax >= 0.0f && tMax >= tMin;
		}

		return false;
	}

	inline glm::vec3 volume() const
	{
		return pMax - pMin;
	}

	inline glm::vec3 centroid() const
	{
		return (pMin + pMax) * 0.5f;
	}

	inline float surfaceArea() const
	{
		glm::vec3 vol = pMax - pMin;
		return vol.x * vol.y * vol.z;
	}

	inline int maxExtent() const
	{
		glm::vec3 vol = pMax - pMin;

		float maxVal = vol.x;
		int maxComponent = 0;
		for (int i = 1; i < 3; i++)
		{
			float component = *((float*)&vol + i);
			if (component > maxVal)
			{
				maxVal = component;
				maxComponent = i;
			}
		}

		return maxComponent;
	}

	glm::vec3 pMin = glm::vec3(0.0f);
	glm::vec3 pMax = glm::vec3(0.0f);
};

#endif
