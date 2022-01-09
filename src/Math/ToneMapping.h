#pragma once

#include <iostream>
#include <cmath>

#include "../glm/glmIncluder.h"

namespace ToneMapping
{
	Vec3f reinhard(const Vec3f &color);
	Vec3f CE(const Vec3f &color);
	Vec3f filmic(const Vec3f &color);
	Vec3f ACES(const Vec3f &color);
}
