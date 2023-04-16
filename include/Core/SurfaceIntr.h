#pragma once

#include <iostream>
#include <memory>

#include "glmIncluder.h"
#include "Core/Sampler.h"

class BSDF;
using BSDFPtr = std::shared_ptr<BSDF>;

struct SurfaceIntr {
	SurfaceIntr() : wo(0.f), uv(0.f), wi(0.f), sampler(nullptr) {}

	SurfaceIntr(const Vec3f& wGiven) :
		wo(wGiven), sampler(nullptr), wi(0.f), uv(0.f) {}

	SurfaceIntr(const Vec3f& wo, const Vec3f& wi) :
		wo(wo), wi(wi), sampler(nullptr), uv(0.f) {}

	SurfaceIntr(const Vec3f &wo, const Vec2f &uv, Sampler* sampler = nullptr) :
		wo(wo), uv(uv), wi(0.f), sampler(sampler) {}

	SurfaceIntr(const Vec3f &wo, const Vec3f &wi, const Vec2f &uv, Sampler* sampler = nullptr) :
		wo(wo), wi(wi), uv(uv), sampler(sampler) {}

	SurfaceIntr swapInOut() const {
		return { wi, wo, uv, sampler };
	}

	Vec3f wo;
	Vec3f wi;
	Vec2f uv;
	Sampler* sampler;
};