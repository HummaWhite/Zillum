#pragma once

#include <iostream>
#include <memory>

#include "../../ext/glmIncluder.h"

class Material;
using MaterialPtr = std::shared_ptr<Material>;

struct SurfaceInfo
{
	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, MaterialPtr mat) :
		uv(uv), ns(ns), material(mat) {}

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, const Vec3f &ng, MaterialPtr mat) :
		uv(uv), ns(ns), ng(ng), material(mat)
	{
		if (glm::dot(ng, ns) < 0)
			this->ng = -ng;
	}

	void flipNormal()
	{
		ns = -ns;
		ng = -ng;
	}

	Vec2f uv;
	Vec3f ns;
	Vec3f ng;
	MaterialPtr material;
};

struct SurfaceInteraction
{
	SurfaceInteraction(const Vec3f &wo, const Vec3f &wi, const Vec3f &ns) :
		wo(wo), wi(wi), ns(ns) {}

	SurfaceInteraction(const Vec3f &wo, const Vec3f &wi, const Vec3f &ns, const Vec3f &ng) :
		wo(wo), wi(wi), ns(ns), ng(ng) {}

	Vec3f wo;
	Vec3f wi;
	Vec3f ns;
	Vec3f ng;
	Vec2f uv;
};