#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>
#include <cmath>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Ray.h"
#include "ClosestHit.h"

struct ClosestHit
{
	ClosestHit(float _dist, bool _hit):
		dist(_dist), hit(_hit) {}

	float dist = 0.0f;
	bool hit = false;
};

struct Hittable
{
	virtual ClosestHit closestHit(const Ray& r) = 0;
};

#endif
