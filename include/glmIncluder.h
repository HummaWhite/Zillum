#pragma once

#define NOMINMAX

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;

using Vec2d = glm::dvec2;
using Vec3d = glm::dvec3;
using Vec4d = glm::dvec4;

using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;

using Mat2f = glm::mat2;
using Mat3f = glm::mat3;
using Mat4f = glm::mat4;

struct Vec2c
{
	Vec2c(float real, float img) : real(real), img(img) {}

	Vec2c operator + (const Vec2c& r) const
	{
		return Vec2c(real + r.real, img + r.img);
	}

	Vec2c operator - (const Vec2c& r) const
	{
		return Vec2c(real - r.real, img - r.img);
	}

	Vec2c operator * (const Vec2c& r) const
	{
		float pReal = real * r.real;
		float pImg = img * r.img;
		return Vec2c(pReal - pImg, pReal + pImg);
	}

	Vec2c operator * (float v) const
	{
		return Vec2c(real * v, img * v);
	}

	Vec2c operator / (const Vec2c& r) const
	{
		float pReal = real * r.real;
		float pImg = img * r.img;
		float scale = 1.f / r.lengthSqr();
		return Vec2c(pReal + pImg, pReal - pImg) * scale;
	}

	Vec2c sqrt() const
	{
		float n = std::sqrt(lengthSqr());
		float t1 = std::sqrt(.5f * (n + std::abs(real)));
		float t2 = .5f * img / t1;

		if (n == 0)
			return Vec2c(0.f, 0.f);

		if (real >= 0)
			return Vec2c(t1, t2);
		else
			return Vec2c(std::abs(t2), std::copysign(t1, img));
	}

	float lengthSqr() const
	{
		return real * real + img * img;
	}

	float real;
	float img;
};