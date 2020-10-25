#ifndef HITTABLE_SHAPES_H
#define HITTABLE_SHAPES_H

#include <utility>
#include "Hittable.h"
#include "../Math/Math.h"

class HittableSphere:
	public Hittable
{
public:
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

	glm::vec3 getRandomPoint()
	{
		RandomGenerator rg;

		float t = rg.get(0.0f, glm::radians(360.0f));
		float p = rg.get(0.0f, glm::radians(180.0f));

		return glm::vec3(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		return glm::normalize(p - center);
	}

	glm::vec3 getCenter() const { return center; }

	float getRadius() const { return radius; }

	AABB bound()
	{
		return AABB(center, radius);
	}

private:
	glm::vec3 center;
	float radius;
};

class HittableTriangle:
	public Hittable
{
public:
	HittableTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c):
		va(a), vb(b), vc(c) {}

	HitInfo closestHit(const Ray &ray)
	{
		const float eps = 1e-6;

		glm::vec3 ab = vb - va;
		glm::vec3 ac = vc - va;

		glm::vec3 o = transform->getInversed(ray.ori);
		glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

		glm::vec3 p = glm::cross(d, ac);

		float det = glm::dot(ab, p);

		if (glm::abs(det) < eps) return { false, 0.0f };

		glm::vec3 ao = o - va;
		if (det < 0.0f)
		{
			ao = -ao;
			det = -det;
		}

		float u = glm::dot(ao, p);
		if (u < 0.0f || u > det) return { false, 0.0f };

		glm::vec3 q = glm::cross(ao, ab);

		float v = glm::dot(d, q);
		if (v < 0.0f || u + v > det) return { false, 0.0f };

		float t = glm::dot(ac, q) / det;
		return { t > 0.0f, t };
	}

	glm::vec3 getRandomPoint()
	{
		RandomGenerator rg;

		float wa = rg.get(0.000001f, 1.0f);
		float wb = rg.get(0.000001f, 1.0f);
		float wc = rg.get(0.000001f, 1.0f);

		glm::vec3 weight = glm::vec3(wa, wb, wc) / (wa + wb + wc);
		glm::vec3 p = va * weight.x + vb * weight.y + vc * weight.z;

		return transform->get(p);
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
		return glm::normalize(transform->getInversedNormal(N));
	}

	bool onPlane(const glm::vec3 &p)
	{
		glm::vec3 invP = transform->getInversed(p);
		return glm::length(glm::cross(va - invP, glm::cross(vb - invP, vc - invP))) < 1e-6;
	}

	glm::vec3 getVa() const { return va; }
	glm::vec3 getVb() const { return vb; }
	glm::vec3 getVc() const { return vc; }

	AABB bound()
	{
		return AABB(transform->get(va), transform->get(vb), transform->get(vc));
	}

private:
	glm::vec3 va, vb, vc;
};

/*
class HittableDisk:
	public Hittable
{
public:
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

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
	}

	glm::vec3 getRandomPoint()
	{
	}

	AABB bound()
	{
	}

private:
	glm::vec3 center;
	float radius;
};
*/

class HittableQuad:
	public Hittable
{
public:
	HittableQuad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c):
		va(a), vb(b), vc(c) {}

	HitInfo closestHit(const Ray &ray)
	{
		glm::vec3 o = transform->getInversed(ray.ori);
		glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

		Ray inversedRay = { o, d };
		glm::vec3 vd = vb + vc - va;

		HitInfo ha = HittableTriangle(va, vb, vc).closestHit(inversedRay);
		HitInfo hb = HittableTriangle(vc, vb, vd).closestHit(inversedRay);

		if (ha.hit) return ha;
		if (hb.hit) return hb;

		return { false, 0.0f };
	}

	glm::vec3 getRandomPoint()
	{
		RandomGenerator rg;

		float la = rg.get(0.0f, 1.0f);
		float lb = rg.get(0.0f, 1.0f);

		return transform->get((vb - va) * la + (vc - va) * lb + va);
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
		return glm::normalize(transform->getInversedNormal(N));
	}

	glm::vec3 getVa() const { return va; }
	glm::vec3 getVb() const { return vb; }
	glm::vec3 getVc() const { return vc; }

	AABB bound()
	{
		glm::vec3 pa = transform->get(va);
		glm::vec3 pb = transform->get(vb);
		glm::vec3 pc = transform->get(vc);
		glm::vec3 pd = pb + pc - pa;
		return AABB(AABB(pa, pb, pc), AABB(pb, pc, pd));
	}

private:
	glm::vec3 va, vb, vc;
};

/*
class HittableMesh:
	public Hittable
{
public:
	HittableMesh(const std::vector<glm::vec3> &_vertices):
		vertices(_vertices) {}

	HittableMesh(const std::vector<glm::vec3> &_vertices, const std::vector<unsigned int> &_indices):
		vertices(_vertices), indices(_indices) {}

	HitInfo closestHit(const Ray &ray)
	{
		glm::vec3 o = ray.ori;
		glm::vec3 d = ray.dir;

		HitInfo hit = { false, 1000.0f };

		bool index = indices.size() > 0;
		int verticesCount = index ? indices.size() : vertices.size();

		for (int i = 0; i < verticesCount; i += 3)
		{
			unsigned int ia = index ? indices[i + 0] : i + 0;
			unsigned int ib = index ? indices[i + 1] : i + 1;
			unsigned int ic = index ? indices[i + 2] : i + 2;

			glm::vec3 pa = glm::vec3(transform * glm::vec4(vertices[ia], 1.0f));
			glm::vec3 pb = glm::vec3(transform * glm::vec4(vertices[ib], 1.0f));
			glm::vec3 pc = glm::vec3(transform * glm::vec4(vertices[ic], 1.0f));

			HitInfo triangleHit = HittableTriangle(pa, pb, pc).closestHit(ray);
		}

		return hit;
	}

	glm::vec3 getRandomPoint()
	{
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
	}

	AABB bound()
	{
	}

private:
	std::vector<glm::vec3> vertices;
	std::vector<unsigned int> indices;
};
*/

#endif
