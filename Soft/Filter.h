#pragma once

#include "glm/glmIncluder.h"
#include "Math/RandomGenerator.h"

namespace Filter
{
	inline Vec3f box(const Vec2f &sample)
	{
		return Vec3f(sample - 0.5f, 1.0f);
	}

	inline Vec3f tent(const Vec2f &sample)
	{
		Vec2f s = sample * 2.0f - 1.0f;
		return Vec3f(s, (1.0f - glm::abs(s.x)) * (1.0f - glm::abs(s.y)));
	}
}
