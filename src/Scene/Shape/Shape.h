#pragma once

#include <utility>
#include "../Hittable.h"
#include "../../Math/Math.h"

class Sphere :
	public Hittable
{
public:
	Sphere(const Vec3f &center, float radius, bool intersectFromInside) :
		center(center), radius(radius), intersectFromInside(intersectFromInside), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	Vec3f uniformSample(const Vec2f &u);

	Vec3f normalGeom(const Vec3f &p);
	float surfaceArea();
	Vec2f surfaceUV(const Vec3f &p);
	Vec3f getCenter() const { return center; }
	float getRadius() const { return radius; }
	AABB bound() { return transform->getTransformedBox(AABB(center, radius)); }

private:
	Vec3f center;
	float radius;
	bool intersectFromInside;
};

class Triangle :
	public Hittable
{
public:
	Triangle(const Vec3f &a, const Vec3f &b, const Vec3f &c) :
		va(a), vb(b), vc(c), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	Vec3f uniformSample(const Vec2f &u);

	Vec3f normalGeom(const Vec3f &p);
	float surfaceArea();
	Vec2f surfaceUV(const Vec3f &p);
	AABB bound();

	std::tuple<Vec3f, Vec3f, Vec3f> vertices() { return { va, vb, vc }; }

protected:
	Vec3f va, vb, vc;
};

class MeshTriangle :
	public Hittable
{
public:
	MeshTriangle(Vec3f *vertices, Vec2f *uvs, Vec3f *normals): 
		triangle(vertices[0], vertices[1], vertices[2]),
		ta(uvs[0]), tb(uvs[1]), tc(uvs[2]),
		na(normals[0]), nb(normals[1]), nc(normals[2]), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray) { return triangle.closestHit(ray); }
	Vec3f uniformSample(const Vec2f &u) { return triangle.uniformSample(u); }

	Vec3f normalGeom(const Vec3f &p) { return triangle.normalGeom(p); }
	Vec3f normalShading(const Vec3f &p) override;
	float surfaceArea() { return triangle.surfaceArea(); }
	Vec2f surfaceUV(const Vec3f &p);
	AABB bound() { return triangle.bound(); }

	void setTransform(TransformPtr trans) override;

private:
	Triangle triangle;
	Vec2f ta, tb, tc;
	Vec3f na, nb, nc;
};

class Quad :
	public Hittable
{
public:
	Quad(const Vec3f &a, const Vec3f &b, const Vec3f &c) :
		va(a), vb(b), vc(c), Hittable(HittableType::Shape) {}

	std::optional<float> closestHit(const Ray &ray);
	Vec3f uniformSample(const Vec2f &u);

	Vec3f normalGeom(const Vec3f &p);
	float surfaceArea();
	Vec2f surfaceUV(const Vec3f &p) { return Triangle(va, vb, vc).surfaceUV(p); }
	AABB bound();

private:
	Vec3f va, vb, vc;
};