#pragma once

#include "../glm/glmIncluder.h"

struct SurfaceInteraction
{
	SurfaceInteraction(const Vec3f &Wo, const Vec3f &Wi, const Vec3f &N):
		Wo(Wo), Wi(Wi), N(N) {}

	Vec3f Wo;
	Vec3f Wi;
	Vec3f N;
	Vec2f uv;
};
