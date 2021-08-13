#pragma once

#include "Environment.h"
#include "../Buffer/Texture.h"
#include "../Math/PiecewiseDistrib.h"

class EnvSingleColor:
	public Environment
{
public:
	EnvSingleColor(const glm::vec3 &color):
		radiance(color) {}
	glm::vec3 getRadiance(const glm::vec3 &dir) { return radiance; }
	float power() { return Math::rgbBrightness(radiance) * 2.0f * Math::square(Math::Pi); }

private:
	glm::vec3 radiance;
};

class EnvSphereMapHDR:
	public Environment
{
public:
	EnvSphereMapHDR(const char *filePath);

	glm::vec3 getRadiance(const glm::vec3 &dir) { return sphereMap.getSpherical(dir); }
	std::pair<glm::vec3, float> importanceSample(const glm::vec2 &u1, const glm::vec2 &u2);
	float pdfLi(const glm::vec3 &Wi) override;
	float power() { return distrib.sum(); }

private:
	float getPortion(const glm::vec3 &Wi);

private:
	Texture sphereMap;
	int w, h;
	PiecewiseIndependent2D distrib;
};

class EnvTest:
	public Environment
{
public:
	EnvTest(const glm::vec3 &color, int _row, int _col):
		radiance(color), row(_row), col(_col) {}
	glm::vec3 getRadiance(const glm::vec3 &dir);

	float power() { return 1.0f; }

private:
	glm::vec3 radiance;
	int row, col;
};
