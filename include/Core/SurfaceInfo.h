#pragma once

#include <iostream>
#include <memory>

#include "../../ext/glmIncluder.h"

class Material;
using MaterialPtr = std::shared_ptr<Material>;

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