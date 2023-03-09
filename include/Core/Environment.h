#pragma once

#include <iostream>
#include <memory>
#include <array>

#include "glmIncluder.h"
#include "Utils/RandomGenerator.h"
#include "Light.h"
#include "Ray.h"
#include "Transform.h"
#include "Texture.h"
#include "PiecewiseDistrib.h"

struct EnvLiSample {
	Vec3f wi;
	Spectrum Li;
	float pdf;
};

class Environment {
public:
	virtual Spectrum radiance(const Vec3f &dir) = 0;
	virtual EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2) = 0;
	virtual float pdfLi(const Vec3f &wi) = 0;
	virtual float power() = 0;
};

using EnvPtr = std::shared_ptr<Environment>;

class EnvSingleColor : public Environment {
public:
	EnvSingleColor(const Spectrum &color) : mRadiance(color) {}

	Spectrum radiance(const Vec3f &dir) { return mRadiance; }
	EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2);
	float pdfLi(const Vec3f &wi);
	float power() { return Math::luminance(mRadiance) * 2.0f * Math::square(Math::Pi); }

private:
	Spectrum mRadiance;
};

class EnvSphereMapHDR : public Environment {
public:
	EnvSphereMapHDR(const char *filePath);

	Spectrum radiance(const Vec3f &dir) { return mSphereMap->getSpherical(dir); }
	EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2);
	float pdfLi(const Vec3f &wi);
	float power() { return mDistrib.sum(); }

private:
	float getPortion(const Vec3f &wi);

private:
	Texture3fPtr mSphereMap;
	int mWidth, mHeight;
	PiecewiseIndependent2D mDistrib;
};