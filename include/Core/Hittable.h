#pragma once

#include <iostream>
#include <optional>
#include <cmath>

#include "Ray.h"
#include "AABB.h"
#include "Transform.h"

enum class HittableType
{
	Object, Light, Shape
};

class Hittable
{
public:
	Hittable(HittableType type) : type(type) {}

	virtual std::optional<float> closestHit(const Ray &ray) = 0;
	virtual Vec3f uniformSample(const Vec2f &u) = 0;
	virtual Vec3f normalGeom(const Vec3f &p) = 0;
	virtual Vec3f normalShading(const Vec3f &p) { return normalGeom(p); }
	virtual float surfaceArea() = 0;
	virtual Vec2f surfaceUV(const Vec3f &p) = 0;
	virtual AABB bound() = 0;

	HittableType getType() const { return type; }

	virtual void setTransform(TransformPtr trans) { transform = trans; }
	virtual void setTransform(const glm::mat4 &trans) { transform->set(trans); }
	TransformPtr getTransform() const { return transform; }

protected:
	TransformPtr transform = std::make_shared<Transform>();
	HittableType type;
};

using HittablePtr = std::shared_ptr<Hittable>;