#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Math.h"

class Transform
{
public:
	Transform(): matrix(1.0f), matInv(1.0f), matInvT(1.0f) {}

	Transform(const glm::mat4 &transform):
		matrix(transform), matInv(glm::inverse(transform))
	{
		matInvT = glm::transpose(matInv);
	}

	void set(const glm::mat4 &transform)
	{
		*this = Transform(transform);
	}

	inline glm::vec3 get(const glm::vec3 &v)
	{
		return glm::vec3(matrix * glm::vec4(v, 1.0f));
	}

	inline glm::vec3 getInversed(const glm::vec3 &v)
	{
		return glm::vec3(matInv * glm::vec4(v, 1.0f));
	}

	inline glm::vec3 getInversedNormal(const glm::vec3 &N)
	{
		return glm::normalize(matInvT * N);
	}

public:
	glm::mat4 matrix;
	glm::mat4 matInv;
	glm::mat3 matInvT;
};

#endif
