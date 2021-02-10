#ifndef ENVIRONMENTS_H
#define ENVIRONMENTS_H

#include "Environment.h"
#include "../Texture.h"
#include "../Math/Transform.h"

class EnvSingleColor:
	public Environment
{
public:
	EnvSingleColor(const glm::vec3 &color):
		radiance(color) {}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		return radiance;
	}

private:
	glm::vec3 radiance;
};

class EnvSphereMapHDR:
	public Environment
{
public:
	EnvSphereMapHDR(const char *filePath)
	{
		sphereMap.loadFloat(filePath);
	}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		return sphereMap.getSpherical(dir);
	}

private:
	Texture sphereMap;
};

class EnvTest:
	public Environment
{
public:
	EnvTest(const glm::vec3 &color, int _row, int _col):
		radiance(color), row(_row), col(_col) {}

	glm::vec3 getRadiance(const glm::vec3 &dir)
	{
		glm::vec2 uv = Transform::sphereToPlane(dir);

		int r = (int)(uv.x * row);
		int c = (int)(uv.y * col);

		return (r & 1) ^ (c & 1) ? radiance : glm::vec3(0.0f);
	}

private:
	glm::vec3 radiance;
	int row, col;
};

#endif
