#pragma once

#include <iostream>
#include <memory>

#include "../glm/glmIncluder.h"
#include "../Material/Material.h"

struct SurfaceInfo
{
	SurfaceInfo(const Vec2f &uv, const Vec3f &N, MaterialPtr mat):
		uv(uv), N(N), mat(mat) {}

	Vec2f uv;
	Vec3f N;
	MaterialPtr mat;
};
