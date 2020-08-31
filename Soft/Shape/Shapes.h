#ifndef SHAPES_H
#define SHAPES_H

#include "Shape.h"

class Sphere:
	public Shape
{
public:
	Sphere(const glm::vec3 &center, float radius, std::shared_ptr<Material> _material):
		Shape(std::make_shared<HittableSphere>(center, radius), _material) {}
};

class Triangle:
	public Shape
{
public:
	Triangle(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc, std::shared_ptr<Material> _material):
		Shape(std::make_shared<HittableTriangle>(va, vb, vc), _material) {}
};

#endif
