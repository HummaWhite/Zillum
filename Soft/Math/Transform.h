#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "Math.h"

class Transform
{
public:
	inline static glm::vec2 sphereToPlane(const glm::vec3 &uv)
	{
		float theta = glm::atan(uv.y, uv.x);
		if (theta < 0.0f) theta += Math::Pi * 2.0f;
		
		float phi = glm::atan(glm::length(glm::vec2(uv)), uv.z);
		if (phi < 0.0f) phi += Math::Pi * 2.0f;

		return { theta / (Math::Pi * 2.0f), phi * Math::PiInv };
	}

	inline static glm::vec3 planeToSphere(const glm::vec2 &uv)
	{
		float theta = uv.x * Math::Pi * 2.0f;
		float phi = uv.y * Math::Pi;
		return { cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi) };
	}

	inline static glm::vec2 toConcentricDisk(const glm::vec2 &uv)
	{
		if (uv.x == 0.0f && uv.y == 0.0f) return glm::vec2(0.0f);
		glm::vec2 v = uv * 2.0f - 1.0f;

		float phi, r;
		if (v.x * v.x > v.y * v.y)
		{
			r = v.x;
			phi = Math::Pi * v.y / v.x * 0.25f;
		}
		else
		{
			r = v.y;
			phi = Math::Pi * 0.5f - Math::Pi * v.x / v.y * 0.25f;
		}
		return glm::vec2(r * glm::cos(phi), r * glm::sin(phi));
	}

	inline static glm::vec3 normalToWorld(const glm::vec3 &N, const glm::vec3 &dir)
	{
		return glm::normalize(Math::TBNMatrix(N) * dir);
	}

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
