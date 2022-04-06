#pragma once

#include <iostream>
#include <memory>

#include "SurfaceInfo.h"
#include "Shape.h"
#include "Material.h"

class Object:
	public Hittable
{
public:
	Object(HittablePtr shape, MaterialPtr material):
		shape(shape), material(material), Hittable(HittableType::Object) {}

	SurfaceInfo surfaceInfo(const Vec3f &x)
	{
		Vec2f uv = shape->surfaceUV(x);
		return SurfaceInfo({ uv.x, 1.0f - uv.y }, shape->normalShading(x), shape->normalGeom(x), material);
	}

	std::optional<float> closestHit(const Ray &r)
	{
		return shape->closestHit(r);
	}

	Vec3f uniformSample(const Vec2f &u)
	{
		return shape->uniformSample(u);
	}

	Vec3f normalGeom(const Vec3f &p)
	{
		return shape->normalGeom(p);
	}

	Vec3f normalShading(const Vec3f &p) override
	{
		return shape->normalShading(p);
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
		mTransform = trans;
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