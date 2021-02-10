#ifndef LIGHT_H
#define LIGHT_H

#include <iostream>
#include <memory>

#include "../Hittable/HittableShapes.h"
#include "../Bound/AABB.h"

class Light
{
public:
	Light(std::shared_ptr<Hittable> _hittable, const glm::vec3 &_radiance, bool directional):
		hittable(_hittable), radiance(_radiance), directional(directional) {}
		
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

	virtual glm::vec3 getRadiance() const { return radiance; }

	virtual glm::vec3 getRadiance(float dist) const
	{
		return radiance / (dist * dist);
	}

	virtual glm::vec3 getRadiance(const glm::vec3 &Wi, const glm::vec3 &N, float dist)
	{
		if (directional) return (glm::length(Wi - N) < 0.1f) ? radiance : glm::vec3(0.0f);
		return radiance * glm::max(0.0f, glm::dot(Wi, N)) / (dist * dist);
	}

	inline Ray getRandomRay()
	{
		glm::vec3 ori = getRandomPoint();
		glm::vec3 N = surfaceNormal(ori);
		glm::vec3 dir = directional ? N : Transform::normalToWorld(N, Math::randHemisphere());
		return { ori + dir * 1e-4f, dir };
	}

	inline AABB bound() const
	{
		return hittable->bound();
	}

protected:
	std::shared_ptr<Hittable> hittable;
	glm::vec3 radiance;
	bool directional;
};

#endif
