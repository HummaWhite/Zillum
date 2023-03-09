#pragma once

#include <iostream>
#include <memory>

#include "glmIncluder.h"

class Material;
using MaterialPtr = std::shared_ptr<Material>;

struct SurfaceInfo {
	SurfaceInfo() = default;

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, MaterialPtr mat) :
		uv(uv), ns(ns), material(mat) {}

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, const Vec3f &ng, MaterialPtr mat) :
		uv(uv), ns(ns), ng(ng), material(mat) {
		if (glm::dot(ng, ns) < 0) {
			this->ng = -ng;
		}
	}

	void flipNormal() {
		ns = -ns;
		ng = -ng;
	}

	Vec2f uv;
	Vec3f ns;
	Vec3f ng;
	MaterialPtr material;
};

struct SurfaceIntr {
	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec2f &uv) :
		n(n), wo(wo), uv(uv) {}

	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, const Vec2f &uv) :
		wo(wo), wi(wi), n(n), uv(uv) {}

	Vec3f n;
	Vec3f wo;
	Vec3f wi;
	Vec2f uv;
};