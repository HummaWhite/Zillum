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

	virtual SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
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

	void setTransform(const glm::mat4 &mat)
	{
		triangle.setTransform(mat);
	}

	std::shared_ptr<Material> getMaterial()
	{
		return material;
	}

	glm::mat4 getTransform() const
	{
		return triangle.getTransform();
	}

protected:
	HittableTriangle triangle;
};

class MeshTriangle:
	public Triangle
{
public:
	MeshTriangle(glm::vec3 *vertices, glm::vec2 *uvs, glm::vec3 *normals, std::shared_ptr<Material> _material):
		Triangle(vertices[0], vertices[1], vertices[2], _material),
		ta(uvs[0]), tb(uvs[1]), tc(uvs[2]), 
		na(normals[0]), nb(normals[1]), nc(normals[2]) {}

	SurfaceInfo surfaceInfo(const glm::vec3 &surfacePoint)
	{
		glm::vec3 va = triangle.getVa();
		glm::vec3 vb = triangle.getVb();
		glm::vec3 vc = triangle.getVc();

		float area = glm::length(glm::cross(vb - va, vc - va));
		float la = glm::length(glm::cross(vb - surfacePoint, vc - surfacePoint)) / area;
		float lb = glm::length(glm::cross(vc - surfacePoint, va - surfacePoint)) / area;
		float lc = glm::length(glm::cross(va - surfacePoint, vb - surfacePoint)) / area;

		glm::mat3 transInv = glm::transpose(glm::inverse(getTransform()));
		glm::vec3 norm = glm::normalize(transInv * (na * la + nb * lb + nc * lc));

		//std::cout << norm.x << "  " << norm.y  << "  " << norm.z << "\n";

		return { ta * la + tb * lb + tc * lc, norm, getMaterial() };
	}

private:
	glm::vec2 ta, tb, tc;
	glm::vec3 na, nb, nc;
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

#endif
