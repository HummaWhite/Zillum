#pragma once

#include <iostream>
#include <memory>

#include "glmIncluder.h"
#include "Core/Sampler.h"

class BSDF;
using BSDFPtr = std::shared_ptr<BSDF>;

const Vec3f LocalUp = Vec3f(0.f, 0.f, 1.f);

struct SurfaceInfo {
	SurfaceInfo() = default;

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, BSDFPtr bsdf) :
		uv(uv), ns(ns), bsdf(bsdf) {}

	SurfaceInfo(const Vec2f &uv, const Vec3f &ns, const Vec3f &ng, BSDFPtr bsdf) :
		uv(uv), ns(ns), ng(ng), bsdf(bsdf) {
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
	BSDFPtr bsdf;
};

struct SurfaceIntr {
	SurfaceIntr() : n(LocalUp), wo(0.f), uv(0.f), wi(0.f), sampler(nullptr) {}

	SurfaceIntr(const Vec3f& wGiven) :
		n(LocalUp), wo(wGiven), sampler(nullptr), wi(0.f), uv(0.f) {}

	SurfaceIntr(const Vec3f& wo, const Vec3f& wi) :
		n(LocalUp), wo(wo), wi(wi), sampler(nullptr), uv(0.f) {}

	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec2f &uv, Sampler* sampler = nullptr) :
		n(n), wo(wo), uv(uv), wi(0.f), sampler(sampler) {}

	SurfaceIntr(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, const Vec2f &uv, Sampler* sampler = nullptr) :
		wo(wo), wi(wi), n(n), uv(uv), sampler(sampler) {}

	std::pair<Vec3f, Vec3f> localDirections() const {
		Mat3f worldToLocal = glm::inverse(Math::localToWorldFrame(n));
		return { worldToLocal * wo, worldToLocal * wi };
	}

	// wo, wi
	SurfaceIntr localized() const {
		auto [wol, wil] = localDirections();
		return { LocalUp, wol, wil, uv, sampler };
	}

	SurfaceIntr swapInOut() const {
		return { n, wo, wi, uv, sampler };
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
	Sampler* sampler;
};