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
		n(n), wo(wo), uv(uv), wi(0.f) {}

	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, const Vec2f &uv) :
		wo(wo), wi(wi), n(n), uv(uv) {}

	std::pair<Vec3f, Vec3f> localDirections() const {
		Mat3f worldToLocal = glm::inverse(Math::localToWorldFrame(n));
		return { worldToLocal * wo, worldToLocal * wi };
	}

	SurfaceIntr localized() const {
		auto [wol, wil] = localDirections();
		return { Vec3f(0.f, 0.f, 1.f), wol, wil, uv };
	}

	bool isLocal() const {
		return n.z > 1.f - 1e-6f;
	}

	float cosWo() const {
		//return isLocal() ? wo.z : glm::dot(n, wo);
		return glm::dot(n, wo);
	}

	float cosWi() const {
		//return isLocal() ? wo.z : glm::dot(n, wi);
		return glm::dot(n, wi);
	}

	Vec3f n;
	Vec3f wo;
	Vec3f wi;
	Vec2f uv;
};