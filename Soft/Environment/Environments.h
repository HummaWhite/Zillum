#ifndef ENVIRONMENTS_H
#define ENVIRONMENTS_H

#include "Environment.h"
#include "../Texture.h"

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

#endif
