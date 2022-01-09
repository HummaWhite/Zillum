#pragma once

#include <iostream>
#include <cmath>

#include "../glm/glmIncluder.h"

struct Ray
{
	Ray() = default;

	Ray(const Vec3f &_ori, const Vec3f &_dir):
		ori(_ori), dir(glm::normalize(_dir)){}

	Vec3f get(float t) const { return ori + dir * t; }

	Vec3f ori;
	Vec3f dir;
};
