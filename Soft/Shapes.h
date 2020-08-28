#ifndef SHAPES_H
#define SHAPES_H

#include "Hittable.h"

struct Sphere:
	public Hittable
{
	Sphere(glm::vec3 _center, float _radius):
		center(_center), radius(_radius) {}

	ClosestHit closestHit(const Ray& ray)
	{
		glm::vec3 d = ray.dir;
		glm::vec3 o = ray.ori;
		glm::vec3 c = center;

		float t = dot(d, c - o) / dot(d, d);
		float r = radius;
		
		float e = glm::length(o + d * t - c);

		if (e > r) return { 0.0f, false };

		return { t - sqrt(r * r - e * e), true };
	}

	glm::vec3 center;
	float radius;
};

#endif
