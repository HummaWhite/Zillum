#pragma once

#include <iostream>
#include <sstream>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Math.h"
#include "../Scene/Ray.h"

struct BoxHit
{
	bool hit;
	float tMin, tMax;
};

struct AABB
{
	AABB() = default;
	AABB(const glm::vec3 _pMin, const glm::vec3 _pMax):
		pMin(_pMin), pMax(_pMax) {}
	AABB(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax):
		pMin(xMin, yMin, zMin), pMax(xMax, yMax, zMax) {}
	AABB(const glm::vec3 &center, float radius):
		pMin(center - glm::vec3(radius)), pMax(center + glm::vec3(radius)) {}

	AABB(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc);
	AABB(const AABB &boundA, const AABB &boundB);

	BoxHit hit(const Ray &ray);
	float volume() const;
	glm::vec3 centroid() const;
	float surfaceArea() const;
	int maxExtent() const;

	std::string toString();
	
	glm::vec3 pMin = glm::vec3(0.0f);
	glm::vec3 pMax = glm::vec3(0.0f);
};
