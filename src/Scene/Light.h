#pragma once

#include <iostream>
#include <optional>
#include <memory>
#include <array>

#include "Shapes.h"

struct LightLiSample
{
	Vec3f Wi;
	Vec3f Li;
	float dist;
	float pdf;
};

struct LightLeSample
{
	Ray ray;
	Vec3f Le;
	float pdfPos;
	float pdfDir;
};

class Light:
	public Hittable
{
public:
	Light(HittablePtr shape, const Vec3f &power, bool delta):
		shape(shape), power(power), Hittable(HittableType::Light) {}

	std::optional<float> closestHit(const Ray &ray) { return shape->closestHit(ray); }

	Vec3f uniformSample(const Vec2f &u) { return shape->uniformSample(u); }
	Vec3f normalGeom(const Vec3f &p) { return shape->normalGeom(p); }
	float surfaceArea() { return shape->surfaceArea(); }
	Vec2f surfaceUV(const Vec3f &p) { return shape->surfaceUV(p); }
	AABB bound() { return shape->bound(); }

	void setTransform(TransformPtr trans) override
	{
		transform = trans;
		shape->setTransform(trans);
	}

	Vec3f getPower(){ return power; }
	float getRgbPower() { return Math::rgbBrightness(power); }
	
	std::optional<LightLiSample> sampleLi(Vec3f ref, Vec2f u);
	float pdfLi(const Vec3f &ref, const Vec3f &y);
	Vec3f Le(Ray ray);
	LightLeSample sampleLe(const std::array<float, 6> &u);

	Ray getRandomRay();

protected:
	HittablePtr shape;
	Vec3f power;
};

using LightPtr = std::shared_ptr<Light>;