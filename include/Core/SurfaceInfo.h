#pragma once

#include <iostream>
#include <memory>

#include "glmIncluder.h"

class BSDF;
using BSDFPtr = std::shared_ptr<BSDF>;

struct SurfaceInfo {
	SurfaceInfo() = default;

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, BSDFPtr mat) :
		uv(uv), ns(ns), material(mat) {}

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, const Vec3f &ng, BSDFPtr mat) :
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
	BSDFPtr material;
};

struct SurfaceIntr {
	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec2f &uv) :
		n(n), wo(wo), uv(uv) {}

	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, const Vec2f &uv) :
		wo(wo), wi(wi), n(n), uv(uv) {}

	SurfaceIntr toLocal() const {
	}

	Vec3f n;
	Vec3f wo;
	Vec3f wi;
	Vec2f uv;
};