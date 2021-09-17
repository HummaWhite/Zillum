#pragma once

#include <iostream>
#include <sstream>

#include "../glm/glmIncluder.h"

#include "../Math/Math.h"
#include "../Scene/Ray.h"

struct BoxHit
{
	bool hit;
	float tMin, tMax;
};

struct AABB
{
	AABB() = default;
	AABB(const Vec3f _pMin, const Vec3f _pMax):
		pMin(_pMin), pMax(_pMax) {}
	AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax):
		pMin(xMin, yMin, zMin), pMax(xMax, yMax, zMax) {}
	AABB(const Vec3f &center, float radius):
		pMin(center - Vec3f(radius)), pMax(center + Vec3f(radius)) {}

	AABB(const Vec3f &va, const Vec3f &vb, const Vec3f &vc);
	AABB(const AABB &boundA, const AABB &boundB);

	BoxHit hit(const Ray &ray);
	float volume() const;
	Vec3f centroid() const;
	float surfaceArea() const;
	int maxExtent() const;

	std::string toString();
	
	Vec3f pMin = Vec3f(0.0f);
	Vec3f pMax = Vec3f(0.0f);
};
