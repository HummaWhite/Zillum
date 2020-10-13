#ifndef LIGHT_H
#define LIGHT_H

#include <iostream>
#include <memory>

#include "../Hittable/HittableShapes.h"
#include "../Bound/AABB.h"

class Light
{
public:
	Light(std::shared_ptr<Hittable> _hittable, const glm::vec3 &_radiance):
		hittable(_hittable), radiance(_radiance) {}
		
	HitInfo closestHit(const Ray &ray)
	{
		return hittable->closestHit(ray);
	}

	glm::vec3 getRandomPoint()
	{
		return hittable->getRandomPoint();
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		return hittable->surfaceNormal(p);
	}

	virtual glm::vec3 getRadiance() const { return radiance; }

	virtual glm::vec3 getRadiance(float dist) const
	{
		return radiance / (dist * dist);
	}

	virtual glm::vec3 getRadiance(const glm::vec3 &Wi, const glm::vec3 &N, float dist)
	{
		return radiance * glm::max(0.0f, glm::dot(Wi, N)) / (dist * dist);
	}

	AABB bound() const
	{
		return hittable->bound();
	}

protected:
	std::shared_ptr<Hittable> hittable;
	glm::vec3 radiance;
};

#endif
