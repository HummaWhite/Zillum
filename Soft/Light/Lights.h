#ifndef LIGHTS_H
#define LIGHTS_H

#include "Light.h"

class SphereLight:
	public Light
{
public:
	SphereLight(const glm::vec3 &center, float radius, const glm::vec3 &radiance):
		Light(std::make_shared<HittableSphere>(center, radius), radiance) {}
};

class TriangleLight:
	public Light
{
public:
	TriangleLight(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc, const glm::vec3 &radiance):
		Light(std::make_shared<HittableTriangle>(va, vb, vc), radiance) {}
};

class QuadLight:
	public Light
{
public:
	QuadLight(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc, const glm::vec3 &radiance):
		Light(std::make_shared<HittableQuad>(va, vb, vc), radiance) {}
};

#endif
