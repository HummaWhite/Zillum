#pragma once

#include <utility>
#include "Hittable.h"
#include "../Math/Math.h"

class Sphere :
	public Hittable
{
public:
	Sphere(const glm::vec3 &center, float radius, bool intersectFromInside) :
		center(center), radius(radius), intersectFromInside(intersectFromInside), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	glm::vec3 uniformSample(const glm::vec2 &u);

	glm::vec3 surfaceNormal(const glm::vec3 &p);
	float surfaceArea();
	glm::vec2 surfaceUV(const glm::vec3 &p);
	glm::vec3 getCenter() const { return center; }
	float getRadius() const { return radius; }
	AABB bound() { return transform->getTransformedBox(AABB(center, radius)); }

private:
	glm::vec3 center;
	float radius;
	bool intersectFromInside;
};

class Triangle :
	public Hittable
{
public:
	Triangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) :
		va(a), vb(b), vc(c), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	glm::vec3 uniformSample(const glm::vec2 &u);

	glm::vec3 surfaceNormal(const glm::vec3 &p);
	float surfaceArea();
	glm::vec2 surfaceUV(const glm::vec3 &p);
	AABB bound();

	std::tuple<glm::vec3, glm::vec3, glm::vec3> vertices() { return { va, vb, vc }; }

protected:
	glm::vec3 va, vb, vc;
};

class MeshTriangle :
	public Hittable
{
public:
	MeshTriangle(glm::vec3 *vertices, glm::vec2 *uvs, glm::vec3 *normals): 
		triangle(vertices[0], vertices[1], vertices[2]),
		ta(uvs[0]), tb(uvs[1]), tc(uvs[2]),
		na(normals[0]), nb(normals[1]), nc(normals[2]), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray) { return triangle.closestHit(ray); }
	glm::vec3 uniformSample(const glm::vec2 &u) { return triangle.uniformSample(u); }

	glm::vec3 surfaceNormal(const glm::vec3 &p);
	float surfaceArea() { return triangle.surfaceArea(); }
	glm::vec2 surfaceUV(const glm::vec3 &p);
	AABB bound() { return triangle.bound(); }

	void setTransform(TransformPtr trans) override;

private:
	Triangle triangle;
	glm::vec2 ta, tb, tc;
	glm::vec3 na, nb, nc;
};

class Quad :
	public Hittable
{
public:
	Quad(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c) :
		va(a), vb(b), vc(c), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	glm::vec3 uniformSample(const glm::vec2 &u);

	glm::vec3 surfaceNormal(const glm::vec3 &p);
	float surfaceArea();
	glm::vec2 surfaceUV(const glm::vec3 &p) { return Triangle(va, vb, vc).surfaceUV(p); }
	AABB bound();

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