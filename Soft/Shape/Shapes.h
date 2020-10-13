#ifndef SHAPES_H
#define SHAPES_H

#include <vector>
#include "Shape.h"

class Sphere:
	public Shape
{
public:
	Sphere(const glm::vec3 &center, float radius, std::shared_ptr<Material> _material):
		Shape(_material), sphere(center, radius) {}

	HitInfo closestHit(const Ray &ray)
	{
		return sphere.closestHit(ray);
	}

	SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
	{
		glm::vec3 p = glm::normalize(surfacePoint - sphere.getCenter());

		glm::vec2 texCoord = Math::sphereToPlane(p);

		return { texCoord, p, material };
	}

	AABB bound()
	{
		return sphere.bound();
	}

private:
	HittableSphere sphere;
};

class Triangle:
	public Shape
{
public:
	Triangle(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc, std::shared_ptr<Material> _material):
		Shape(_material), triangle(va, vb, vc) {}

	HitInfo closestHit(const Ray &ray)
	{
		return triangle.closestHit(ray);
	}

	SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
	{
		glm::vec3 va = triangle.getVa();
		glm::vec3 vb = triangle.getVb();
		glm::vec3 vc = triangle.getVc();

		glm::vec3 norm = glm::normalize(glm::cross(vb - va, vc - va));

		return { glm::vec2(0.0f), norm, material };
	}

	AABB bound()
	{
		return triangle.bound();
	}

private:
	HittableTriangle triangle;
};

class Quad:
	public Shape
{
public:
	Quad(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc, std::shared_ptr<Material> _material):
		Shape(_material), quad(va, vb, vc) {}

	HitInfo closestHit(const Ray &ray)
	{
		return quad.closestHit(ray);
	}

	SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
	{
		glm::vec3 va = quad.getVa();
		glm::vec3 vb = quad.getVb();
		glm::vec3 vc = quad.getVc();

		glm::vec3 norm = glm::normalize(glm::cross(vb - va, vc - va));

		return { glm::vec2(0.0f), norm, material };
	}

	AABB bound()
	{
		return quad.bound();
	}

private:
	HittableQuad quad;
};

/*
class Mesh:
	public Shape
{
public:
	Mesh(const std::vector<glm::vec3> &vertices, std::shared_ptr<Material> _material):
		Shape(_material), mesh(vertices) {}

	Mesh(const std::vector<glm::vec3> &vertices, const std::vector<unsigned int> &indices, std::shared_ptr<Material> _material):
		Shape(_material), mesh(vertices, indices) {}

	HitInfo closestHit(const Ray &ray)
	{
	}

	SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
	{
	}

	AABB bound()
	{
		return mesh.bound();
	}

private:
	HittableMesh mesh;
};
*/

#endif
