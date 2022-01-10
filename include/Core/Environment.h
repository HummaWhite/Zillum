#pragma once

#include <iostream>
#include <memory>
#include <array>

#include "../../ext/glmIncluder.h"
#include "../Utils/RandomGenerator.h"
#include "Light.h"
#include "Ray.h"
#include "Transform.h"
#include "Texture.h"
#include "PiecewiseDistrib.h"

struct EnvLiSample
{
	Vec3f Wi;
	Vec3f Li;
	float pdf;
};

class Environment
{
public:
	virtual Vec3f getRadiance(const Vec3f &dir) = 0;
	virtual EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2) = 0;
	virtual float pdfLi(const Vec3f &Wi) = 0;
	virtual LightLeSample sampleLe(float radius, const std::array<float, 6> &u) = 0;
	virtual float power() = 0;
};

using EnvPtr = std::shared_ptr<Environment>;

class EnvSingleColor:
	public Environment
{
public:
	EnvSingleColor(const Vec3f &color) : radiance(color) {}

	Vec3f getRadiance(const Vec3f &dir) { return radiance; }
	EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2);
	float pdfLi(const Vec3f &Wi);
	LightLeSample sampleLe(float radius, const std::array<float, 6> &u);
	float power() { return Math::rgbBrightness(radiance) * 2.0f * Math::square(Math::Pi); }

private:
	Vec3f radiance;
};

class EnvSphereMapHDR:
	public Environment
{
public:
	EnvSphereMapHDR(const char *filePath);

	Vec3f getRadiance(const Vec3f &dir) { return sphereMap.getSpherical(dir); }
	EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2);
	float pdfLi(const Vec3f &Wi);
	LightLeSample sampleLe(float radius, const std::array<float, 6> &u);
	float power() { return distrib.sum(); }

private:
	float getPortion(const Vec3f &Wi);

private:
	Texture sphereMap;
	int w, h;
	PiecewiseIndependent2D distrib;
};