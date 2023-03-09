#pragma once

#include <iostream>
#include <optional>
#include <memory>
#include <array>

#include "Shape.h"

struct LightLiSample {
	Vec3f wi;
	Spectrum Li;
	float dist;
	float pdf;
};

struct LightLeSample {
	Ray ray;
	Spectrum Le;
	float pdfPos;
	float pdfDir;
};

struct LightPdf {
	float pdfPos;
	float pdfDir;
};

class Light : public Hittable {
public:
	Light(HittablePtr shape, const Spectrum &power, bool delta):
		shape(shape), mPower(power), Hittable(HittableType::Light) {}

	std::optional<float> closestHit(const Ray &ray) { return shape->closestHit(ray); }

	Vec3f uniformSample(const Vec2f &u) { return shape->uniformSample(u); }
	Vec3f normalGeom(const Vec3f &p) { return shape->normalGeom(p); }
	float surfaceArea() { return shape->surfaceArea(); }
	Vec2f surfaceUV(const Vec3f &p) { return shape->surfaceUV(p); }
	AABB bound() { return shape->bound(); }

	void setTransform(TransformPtr trans) override {
		mTransform = trans;
		shape->setTransform(trans);
	}

	Spectrum getPower(){ return mPower; }
	float luminance() { return Math::luminance(mPower); }
	
	std::optional<LightLiSample> sampleLi(Vec3f refPoint, Vec2f u);
	float pdfLi(const Vec3f &ref, const Vec3f &y);
	Spectrum Le(Ray ray);
	LightLeSample sampleLe(const std::array<float, 4> &u);
	LightPdf pdfLe(const Ray &ray);

protected:
	HittablePtr shape;
	Vec3f mPower;
};

using LightPtr = std::shared_ptr<Light>;