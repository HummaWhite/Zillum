#pragma once

#include <iostream>
#include <sstream>

#include "../../ext/glmIncluder.h"

#include "Math.h"
#include "Ray.h"

struct BoxHit
{
	bool hit;
	float tMin, tMax;
};

struct AABB
{
	AABB() : pMin(1e30f), pMax(-1e30f) {}
	AABB(const Vec3f &p) : pMin(p), pMax(p) {}
	AABB(const Vec3f &pMin, const Vec3f &pMax):
		pMin(pMin), pMax(pMax) {}
	AABB(const Vec3f &center, float radius):
		pMin(center - Vec3f(radius)), pMax(center + Vec3f(radius)) {}

	AABB(const Vec3f &va, const Vec3f &vb, const Vec3f &vc);
	AABB(const AABB &boundA, const AABB &boundB);

	void expand(const AABB& rhs);
	BoxHit hit(const Ray &ray);
	float volume() const;
	Vec3f centroid() const;
	float surfaceArea() const;
	int maxExtent() const;

	std::string toString();
	
	Vec3f pMin;
	Vec3f pMax;
};
