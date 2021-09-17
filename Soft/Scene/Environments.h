#pragma once

#include "Environment.h"
#include "../Buffer/Texture.h"
#include "../Math/PiecewiseDistrib.h"

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