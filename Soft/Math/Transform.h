#pragma once

#include <memory>

#include "Math.h"
#include "../Accelerator/AABB.h"

class Transform
{
public:
	Transform(): matrix(1.0f), matInv(1.0f), matInvT(1.0f) {}
	Transform(const glm::mat4 &transform):
		matrix(transform), matInv(glm::inverse(transform)), matInvT(glm::transpose(matInv)) {}

	void set(const glm::mat4 &transform) { *this = Transform(transform); }
	glm::vec3 get(const glm::vec3 &v) { return glm::vec3(matrix * glm::vec4(v, 1.0f)); }
	glm::vec3 getInversed(const glm::vec3 &v) { return glm::vec3(matInv * glm::vec4(v, 1.0f)); }
	glm::vec3 getInversedNormal(const glm::vec3 &N) { return glm::normalize(matInvT * N); }

	AABB getTransformedBox(const AABB &box);

	static glm::vec2 sphereToPlane(const glm::vec3 &uv);
	static glm::vec3 planeToSphere(const glm::vec2 &uv);
	static glm::vec2 toConcentricDisk(const glm::vec2 &uv);
	static glm::vec3 normalToWorld(const glm::vec3 &N, const glm::vec3 &dir);

public:
	glm::mat4 matrix;
	glm::mat4 matInv;
	glm::mat3 matInvT;
};

typedef std::shared_ptr<Transform> TransformPtr;

namespace Math
{
	std::pair<glm::vec3, float> sampleHemisphereCosine(const glm::vec3 &N, const glm::vec2 &u);
}