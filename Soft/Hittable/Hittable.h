#ifndef HITTABLE_H
#define HITTABLE_H

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Ray.h"
#include "../Hit/HitInfo.h"
#include "../Bound/AABB.h"

class Hittable
{
public:
	virtual HitInfo closestHit(const Ray &r) = 0;
	
	virtual glm::vec3 getRandomPoint() = 0;

	void setTransform(const glm::mat4& trans)
	{
		transform = trans;
	}

	virtual glm::vec3 surfaceNormal(const glm::vec3 &p) = 0;

	virtual AABB bound() = 0;

	glm::mat4 getTransform() const { return transform; }

protected:
	glm::mat4 transform = glm::mat4(1.0f);
};

#endif
