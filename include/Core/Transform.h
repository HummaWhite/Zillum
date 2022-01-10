#pragma once

#include <memory>

#include "Math.h"
#include "AABB.h"

class Transform
{
public:
	Transform(): matrix(1.0f), matInv(1.0f), matInvT(1.0f) {}
	Transform(const Mat4f &transform):
		matrix(transform), matInv(glm::inverse(transform)), matInvT(glm::transpose(matInv)) {}

	void set(const Mat4f &transform) { *this = Transform(transform); }
	Vec3f get(const Vec3f &v) { return Vec3f(matrix * glm::vec4(v, 1.0f)); }
	Vec3f getInversed(const Vec3f &v) { return Vec3f(matInv * glm::vec4(v, 1.0f)); }
	Vec3f getInversedNormal(const Vec3f &N) { return glm::normalize(matInvT * N); }

	AABB getTransformedBox(const AABB &box);

	static Vec2f sphereToPlane(const Vec3f &uv);
	static Vec3f planeToSphere(const Vec2f &uv);
	static Vec2f toConcentricDisk(const Vec2f &uv);
	static Vec3f normalToWorld(const Vec3f &N, const Vec3f &dir);

public:
	Mat4f matrix;
	Mat4f matInv;
	Mat3f matInvT;
};

using TransformPtr = std::shared_ptr<Transform>;

namespace Math
{
	std::pair<Vec3f, float> sampleHemisphereCosine(const Vec3f &N, const Vec2f &u);
}