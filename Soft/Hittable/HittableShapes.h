#ifndef HITTABLE_SHAPES_H
#define HITTABLE_SHAPES_H

#include "Hittable.h"
#include "../Math/Math.h"

struct HittableSphere:
	Hittable
{
	HittableSphere(const glm::vec3 &_center, float _radius):
		center(_center), radius(_radius) {}

	HitInfo closestHit(const Ray &ray)
	{
		glm::vec3 d = ray.dir;
		glm::vec3 o = ray.ori;
		glm::vec3 c = center;

		float t = dot(d, c - o) / dot(d, d);
		float r = radius;
		
		float e = glm::length(o + d * t - c);
		if (e > r) return { false, 0.0f };

		float res = t - sqrt(r * r - e * e);
		return { res >= 0.0f, res };
	}

	glm::vec3 surfaceNormal(const glm::vec3 &surfacePoint)
	{
		return glm::normalize(surfacePoint - center);
	}

	glm::vec3 getRandomPoint()
	{
		RandomGenerator rg;

		float t = rg.get(0.0f, glm::radians(360.0f));
		float p = rg.get(0.0f, glm::radians(180.0f));

		return glm::vec3(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
	}

	glm::vec3 center;
	float radius;
};

struct HittableTriangle:
	Hittable
{
	HittableTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c):
		va(a), vb(b), vc(c) {}

	HitInfo closestHit(const Ray &ray)
	{
		glm::vec3 a = glm::vec3(transform * glm::vec4(va, 1.0f));
		glm::vec3 b = glm::vec3(transform * glm::vec4(vb, 1.0f));
		glm::vec3 c = glm::vec3(transform * glm::vec4(vc, 1.0f));

		glm::vec3 norm = glm::normalize(glm::cross(b - a, c - a));

		glm::vec3 o = ray.ori;
		glm::vec3 d = ray.dir;

		const float eps = 1e-6;

		if (glm::abs(glm::dot(d, norm)) < eps) return { false, 0.0f };
		if (glm::length(va - o) < eps) return { true, 0.0f };

		float t = glm::dot(va - o, norm) / glm::dot(d, norm);
		glm::vec3 p = o + d * t;

		glm::vec3 ea = glm::normalize(a - p);
		glm::vec3 eb = glm::normalize(b - p);
		glm::vec3 ec = glm::normalize(c - p);

		glm::vec3 crA = glm::cross(eb, ec);
		glm::vec3 crB = glm::cross(ec, ea);
		glm::vec3 crC = glm::cross(ea, eb);

		float da = glm::dot(crB, crC);
		float db = glm::dot(crC, crA);
		float dc = glm::dot(crA, crB);

		if (da >= 0.0f && db >= 0.0f && dc >= 0.0f) return { t > 0, t };
		return { false, 0.0f };
	}

	glm::vec3 surfaceNormal(const glm::vec3 &surfacePoint)
	{
		return glm::normalize(glm::cross(vb - va, vc - va));
	}

	glm::vec3 getRandomPoint()
	{
		RandomGenerator rg;

		float wa = rg.get(0.000001f, 1.0f);
		float wb = rg.get(0.000001f, 1.0f);
		float wc = rg.get(0.000001f, 1.0f);

		glm::vec3 weight = glm::vec3(wa, wb, wc) / (wa + wb + wc);

		return va * weight.x + vb * weight.y + vc * weight.z;
	}

	glm::vec3 va, vb, vc;
};

struct HittableDisk:
	Hittable
{
	HittableDisk(const glm::vec3 &_center, float _radius):
		center(_center), radius(_radius) {}

	HitInfo closestHit(const Ray &ray)
	{
		glm::vec3 o = ray.ori;
		glm::vec3 d = ray.dir;

		glm::mat3 normMatrix = glm::transpose(glm::inverse(transform));
		glm::vec3 norm = normMatrix * glm::vec3(0.0f, 0.0f, 1.0f);

		const float eps = 1e-6;
	}

	glm::vec3 surfaceNormal(const glm::vec3 &surfacePoint)
	{
	}

	glm::vec3 center;
	float radius;
};

#endif
