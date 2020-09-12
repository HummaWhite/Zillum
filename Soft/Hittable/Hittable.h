#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Ray.h"
#include "../SurfaceInfo.h"

struct HitInfo
{
	bool hit;
	float dist;
};

struct Hittable
{
	virtual HitInfo closestHit(const Ray &r) = 0;
	virtual glm::vec3 surfaceNormal(const glm::vec3 &surfacePoint) = 0;
	virtual glm::vec3 getRandomPoint() = 0;

	glm::mat4 transform = glm::mat4(1.0f);
};

#endif
