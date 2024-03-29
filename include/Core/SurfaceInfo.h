#pragma once

#include <iostream>
#include <memory>

#include "Core/BSDF.h"

struct SurfaceInfo {
	SurfaceInfo() = default;

	SurfaceInfo(const Vec2f& uv, const Vec3f& ns, BSDFPtr bsdf) :
		uv(uv), ns(ns), bsdf(bsdf) {}

	SurfaceInfo(const Vec2f& uv, const Vec3f& ns, const Vec3f& ng, BSDFPtr bsdf) :
		uv(uv), ns(ns), ng(ng), bsdf(bsdf) {
		if (glm::dot(ng, ns) < 0) {
			this->ng = -ng;
		}
	}

	void flipNormal() {
		ns = -ns;
		ng = -ng;
	}

	Spectrum f(Vec3f n, Vec3f wo, Vec3f wi, Sampler* sampler, TransportMode mode = TransportMode::Radiance) {
		wo = Transform::worldToLocal(n, wo);
		wi = Transform::worldToLocal(n, wi);
		return bsdf->bsdf(wo, wi, uv, mode, sampler);
	}

	float pdf(Vec3f n, Vec3f wo, Vec3f wi, Sampler* sampler, TransportMode mode = TransportMode::Radiance) {
		wo = Transform::worldToLocal(n, wo);
		wi = Transform::worldToLocal(n, wi);
		return bsdf->pdf(wo, wi, uv, mode, sampler);
	}

	std::optional<BSDFSample> sample(Vec3f n, Vec3f wo, Sampler* sampler, TransportMode mode = TransportMode::Radiance) {
		wo = Transform::worldToLocal(n, wo);
		auto sample = bsdf->sample(wo, uv, mode, sampler);
		if (!sample) {
			return std::nullopt;
		}
		else {
			sample->w = Transform::localToWorld(n, sample->w);
			return sample;
		}
	}

	Vec2f uv;
	Vec3f ns;
	Vec3f ng;
	BSDFPtr bsdf;
};