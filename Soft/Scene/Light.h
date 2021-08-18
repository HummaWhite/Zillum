#pragma once

#include <iostream>
#include <optional>
#include <memory>
#include <array>

#include "Shapes.h"

struct LightLiSample
{
	glm::vec3 Wi;
	glm::vec3 Li;
	float dist;
	float pdf;
};

struct LightLeSample
{
	Ray ray;
	glm::vec3 Le;
	float pdfPos;
	float pdfDir;
};

class Light:
	public Hittable
{
public:
	Light(HittablePtr shape, const glm::vec3 &power, bool delta):
		shape(shape), power(power), Hittable(HittableType::Light) {}

	std::optional<float> closestHit(const Ray &ray) { return shape->closestHit(ray); }

	glm::vec3 uniformSample(const glm::vec2 &u) { return shape->uniformSample(u); }
	glm::vec3 surfaceNormal(const glm::vec3 &p) { return shape->surfaceNormal(p); }
	float surfaceArea() { return shape->surfaceArea(); }
	glm::vec2 surfaceUV(const glm::vec3 &p) { return shape->surfaceUV(p); }
	AABB bound() { return shape->bound(); }

	void setTransform(TransformPtr trans) override
	{
		transform = trans;
		shape->setTransform(trans);
	}

	glm::vec3 getPower(){ return power; }
	float getRgbPower() { return Math::rgbBrightness(power); }
	
	std::optional<LightLiSample> sampleLi(glm::vec3 x, glm::vec2 u);
	float pdfLi(const glm::vec3 &x, const glm::vec3 &y);
	glm::vec3 Le(Ray ray);
	LightLeSample sampleLe(const std::array<float, 6> &u);

	Ray getRandomRay();

protected:
	HittablePtr shape;
	glm::vec3 power;
};

typedef std::shared_ptr<Light> LightPtr;