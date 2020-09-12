#ifndef LIGHT_H
#define LIGHT_H

#include <iostream>
#include <memory>

#include "../Hittable/HittableShapes.h"

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

	glm::vec3 getRadiance() { return radiance; }

protected:
	std::shared_ptr<Hittable> hittable;
	glm::vec3 radiance;
};

#endif
