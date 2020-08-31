#ifndef SHAPE_H
#define SHAPE_H

#include <iostream>
#include <memory>

#include "../SurfaceInfo.h"
#include "../Hittable/HittableShapes.h"
#include "../Material/Material.h"

class Shape
{
public:
	Shape(std::shared_ptr<Hittable> _hittable, std::shared_ptr<Material> _material):
		hittable(_hittable), material(_material) {}
		
	HitInfo closestHit(const Ray &ray)
	{
		return hittable->closestHit(ray);
	}

	SurfaceInfo surfaceInfo(const glm::vec3 &point)
	{
		return { point, hittable->surfaceNormal(point), material };
	}

protected:
	std::shared_ptr<Hittable> hittable;
	std::shared_ptr<Material> material;
};

#endif
