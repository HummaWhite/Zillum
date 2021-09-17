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
		shape(shape), material(material), Hittable(HittableType::Object) {}

	SurfaceInfo surfaceInfo(const Vec3f &x)
	{
		return { shape->surfaceUV(x), shape->surfaceNormal(x), material };
	}

	std::optional<float> closestHit(const Ray &r)
	{
		return shape->closestHit(r);
	}

	Vec3f uniformSample(const Vec2f &u)
	{
		return shape->uniformSample(u);
	}

	Vec3f surfaceNormal(const Vec3f &p)
	{
		return shape->surfaceNormal(p);
	}

	float surfaceArea()
	{
		return shape->surfaceArea();
	}

	Vec2f surfaceUV(const Vec3f &p)
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

using ObjectPtr = std::shared_ptr<Object>;