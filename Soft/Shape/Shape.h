#ifndef SHAPE_H
#define SHAPE_H

#include <iostream>
#include <memory>

#include "../Hit/SurfaceInfo.h"
#include "../Hittable/HittableShapes.h"
#include "../Material/Material.h"
#include "../Bound/AABB.h"

class Shape
{
public:
	Shape(std::shared_ptr<Material> _material):
		material(_material) {}
		
	virtual HitInfo closestHit(const Ray &ray) = 0;

	virtual SurfaceInfo surfaceInfo(const glm::vec3 &point) = 0;

	virtual AABB bound() = 0;

protected:
	std::shared_ptr<Material> material;
};

#endif
