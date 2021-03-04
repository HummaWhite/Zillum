#ifndef LIGHT_H
#define LIGHT_H

#include <iostream>
#include <memory>

#include "../Hittable/HittableShapes.h"
#include "../Bound/AABB.h"

class Light
{
public:
	Light(std::shared_ptr<Hittable> hittable, const glm::vec3 &power, bool directional):
		hittable(hittable), power(power) {}
		
	inline HitInfo closestHit(const Ray &ray)
	{
		return hittable->closestHit(ray);
	}

	inline glm::vec3 getRandomPoint()
	{
		return hittable->getRandomPoint();
	}

	inline glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		return hittable->surfaceNormal(p);
	}

	inline float surfaceArea() const
	{
		return hittable->surfaceArea();
	}

	inline glm::vec3 getPower() const { return power; }

	inline glm::vec3 getRadiance() const
	{
		//return power * Math::PiInv / hittable->surfaceArea();
		return power;
	}

	inline Ray getRandomRay()
	{
		glm::vec3 ori = getRandomPoint();
		glm::vec3 N = surfaceNormal(ori);
		glm::vec3 dir = Transform::normalToWorld(N, Math::randHemisphere());
		return { ori + dir * 1e-4f, dir };
	}

	inline AABB bound() const
	{
		return hittable->bound();
	}

protected:
	std::shared_ptr<Hittable> hittable;
	glm::vec3 power;
};

#endif
