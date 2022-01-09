#pragma once

#include "../glm/glmIncluder.h"

struct SurfaceInteraction
{
	SurfaceInteraction(const Vec3f &Wo, const Vec3f &Wi, const Vec3f &Ns) :
		Wo(Wo), Wi(Wi), Ns(Ns) {}

	SurfaceInteraction(const Vec3f &Wo, const Vec3f &Wi, const Vec3f &Ns, const Vec3f &Ng) :
		Wo(Wo), Wi(Wi), Ns(Ns), Ng(Ng) {}

	Vec3f Wo;
	Vec3f Wi;
	Vec3f Ns;
	Vec3f Ng;
	Vec2f uv;
};
