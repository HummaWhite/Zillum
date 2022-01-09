#pragma once

#include <iostream>
#include <memory>

#include "../glm/glmIncluder.h"
#include "../Material/Material.h"

struct SurfaceInfo
{
	SurfaceInfo(const Vec2f &uv, const Vec3f &Ns, MaterialPtr mat) :
		uv(uv), Ns(Ns), mat(mat) {}

	SurfaceInfo(const Vec2f &uv, const Vec3f &Ns, const Vec3f &Ng, MaterialPtr mat) :
		uv(uv), Ns(Ns), Ng(Ng), mat(mat) {}

	Vec2f uv;
	Vec3f Ns;
	Vec3f Ng;
	MaterialPtr mat;
};
