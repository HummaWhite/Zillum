#pragma once

#include <utility>
#include "Hittable.h"
#include "../Math/Math.h"

class Sphere:
	public Hittable
{
public:
	Sphere(const glm::vec3 &center, float radius, bool intersectFromInside):
		center(center), radius(radius), intersectFromInside(intersectFromInside) {}

	std::optional<float> closestHit(const Ray &ray)
	{
		glm::vec3 o = transform->getInversed(ray.ori);
		glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;
		glm::vec3 c = center;

		float t = dot(d, c - o) / dot(d, d);
		float r = radius;
		
		float e = glm::length(o + d * t - c);
		if (e > r) return std::nullopt;

		float q = sqrt(glm::max(r * r - e * e, 0.0f));
		// 想不到吧，r * r - e * e还是可能小于0
		if (glm::length(o - c) < r)
		{
			if (!intersectFromInside) return std::nullopt;
			float res = t + q;
			return res >= 0 ? res : std::optional<float>();
		}
		float res = t - q;
		return res >= 0 ? res : std::optional<float>();
	}

	glm::vec3 uniformSample(const glm::vec2 &u)
	{
		float t = Math::Pi * 2.0f * u.x;
		float p = Math::Pi * u.y;

		return glm::vec3(cos(t) * sin(p), sin(t) * sin(p), cos(p)) * radius + center;
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		return transform->getInversedNormal(glm::normalize(p - center));
	}

	float surfaceArea()
	{
		return 4.0f * Math::Pi * radius * radius;
	}

	glm::vec2 surfaceUV(const glm::vec3 &p)
	{
		auto oriP = transform->getInversed(p);
		return Transform::sphereToPlane(glm::normalize(oriP - center));
	}

	glm::vec3 getCenter() const { return center; }

	float getRadius() const { return radius; }

	AABB bound()
	{
		return transform->getTransformedBox(AABB(center, radius));
	}

private:
	glm::vec3 center;
	float radius;
	bool intersectFromInside;
};

class Triangle:
	public Hittable
{
public:
	Triangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c):
		va(a), vb(b), vc(c) {}

	std::optional<float> closestHit(const Ray &ray)
	{
		const float eps = 1e-6;

		glm::vec3 ab = vb - va;
		glm::vec3 ac = vc - va;

		glm::vec3 o = transform->getInversed(ray.ori);
		glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

		glm::vec3 p = glm::cross(d, ac);

		float det = glm::dot(ab, p);

		if (glm::abs(det) < eps) return std::nullopt;

		glm::vec3 ao = o - va;
		if (det < 0.0f)
		{
			ao = -ao;
			det = -det;
		}

		float u = glm::dot(ao, p);
		if (u < 0.0f || u > det) return std::nullopt;

		glm::vec3 q = glm::cross(ao, ab);

		float v = glm::dot(d, q);
		if (v < 0.0f || u + v > det) return std::nullopt;

		float t = glm::dot(ac, q) / det;
		return t > 0.0f ? t : std::optional<float>();
	}

	glm::vec3 uniformSample(const glm::vec2 &u)
	{
		float r = glm::sqrt(u.y);
		float a = 1.0f - r;
		float b = u.x * r;

		glm::vec3 p = va * (1.0f - a - b) + vb * a + vc * b;
		return transform->get(p);
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
		return glm::normalize(transform->getInversedNormal(N));
	}

	float surfaceArea()
	{
		return 0.5f * glm::length(glm::cross(vc - va, vb - va));
	}

	glm::vec2 surfaceUV(const glm::vec3 &p)
	{
		glm::vec3 oriP = transform->getInversed(p);

		float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
		float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
		float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
		return glm::vec2(u, v);
	}

	std::tuple<glm::vec3, glm::vec3, glm::vec3> vertices()
	{
		return { va, vb, vc };
	}

	AABB bound()
	{
		return AABB(transform->get(va), transform->get(vb), transform->get(vc));
	}

protected:
	glm::vec3 va, vb, vc;
};

class MeshTriangle:
	public Hittable
{
public:
	MeshTriangle(glm::vec3 *vertices, glm::vec2 *uvs, glm::vec3 *normals): 
		triangle(vertices[0], vertices[1], vertices[2]),
		ta(uvs[0]), tb(uvs[1]), tc(uvs[2]),
		na(normals[0]), nb(normals[1]), nc(normals[2]) {}

	std::optional<float> closestHit(const Ray &ray)
	{
		return triangle.closestHit(ray);
	}

	glm::vec3 uniformSample(const glm::vec2 &u)
	{
		return triangle.uniformSample(u);
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		auto [va, vb, vc] = triangle.vertices();
		glm::vec3 oriP = triangle.getTransform()->getInversed(p);

		float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
		float la = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
		float lb = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;
		float lc = glm::length(glm::cross(va - oriP, vb - oriP)) * areaInv;

		return glm::normalize(triangle.getTransform()->getInversedNormal(na * la + nb * lb + nc * lc));
	}

	float surfaceArea()
	{
		return triangle.surfaceArea();
	}

	glm::vec2 surfaceUV(const glm::vec3 &p)
	{
		auto [va, vb, vc] = triangle.vertices();
		glm::vec3 oriP = triangle.getTransform()->getInversed(p);

		float areaInv = 1.0f / glm::length(glm::cross(vb - va, vc - va));
		float u = glm::length(glm::cross(vb - oriP, vc - oriP)) * areaInv;
		float v = glm::length(glm::cross(vc - oriP, va - oriP)) * areaInv;

		return glm::vec2(u, v);
	}

	void setTransform(std::shared_ptr<Transform> trans) override
	{
		transform = trans;
		triangle.setTransform(trans);
	}

	AABB bound()
	{
		return triangle.bound();
	}

private:
	Triangle triangle;
	glm::vec2 ta, tb, tc;
	glm::vec3 na, nb, nc;
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

class Quad:
	public Hittable
{
public:
	Quad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c):
		va(a), vb(b), vc(c) {}

	std::optional<float> closestHit(const Ray &ray)
	{
		glm::vec3 o = transform->getInversed(ray.ori);
		glm::vec3 d = transform->getInversed(ray.ori + ray.dir) - o;

		Ray inversedRay = { o, d };
		glm::vec3 vd = vb + vc - va;

		auto ha = Triangle(va, vb, vc).closestHit(inversedRay);
		auto hb = Triangle(vc, vb, vd).closestHit(inversedRay);

		if (ha.has_value()) return ha;
		if (hb.has_value()) return hb;

		return std::nullopt;
	}

	inline glm::vec3 uniformSample(const glm::vec2 &u)
	{
		return transform->get((vb - va) * u.x + (vc - va) * u.y + va);
	}

	inline glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		glm::vec3 N = glm::normalize(glm::cross(vb - va, vc - va));
		return glm::normalize(transform->getInversedNormal(N));
	}

	float surfaceArea()
	{
		return glm::length(glm::cross(vc - va, vb - va));
	}

	glm::vec2 surfaceUV(const glm::vec3 &p)
	{
		return Triangle(va, vb, vc).surfaceUV(p);
	}

	inline AABB bound()
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
