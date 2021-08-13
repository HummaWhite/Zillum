#pragma once

#include <iostream>
#include <memory>

#include "../Surface/SurfaceInfo.h"
#include "Shapes.h"
#include "../Material/Material.h"

class Object:
	public Hittable
{
public:
	Object(HittablePtr shape, MaterialPtr material):
		shape(shape), material(material) {}

	SurfaceInfo surfaceInfo(const glm::vec3 &x)
	{
		return { shape->surfaceUV(x), shape->surfaceNormal(x), material };
	}

	std::optional<float> closestHit(const Ray &r)
	{
		return shape->closestHit(r);
	}

	glm::vec3 uniformSample(const glm::vec2 &u)
	{
		return shape->uniformSample(u);
	}

	glm::vec3 surfaceNormal(const glm::vec3 &p)
	{
		return shape->surfaceNormal(p);
	}

	float surfaceArea()
	{
		return shape->surfaceArea();
	}

	glm::vec2 surfaceUV(const glm::vec3 &p)
	{
		return shape->surfaceUV(p);
	}

	void setTransform(TransformPtr trans) override
	{
		transform = trans;
		shape->setTransform(trans);
	}

	AABB bound()
	{
		return shape->bound();
	}

protected:
	HittablePtr shape;
	MaterialPtr material;
};

typedef std::shared_ptr<Object> ObjectPtr;