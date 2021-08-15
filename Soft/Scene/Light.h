#pragma once

#include <iostream>
#include <optional>
#include <memory>
#include <array>

#include "Shapes.h"

struct LightLiSample
{
	glm::vec3 Wi;
	glm::vec3 weight;
	float dist;
	float pdf;
};

typedef std::tuple<Ray, glm::vec3, float> LightLeSample;

class Light:
	public Hittable
{
public:
	Light(HittablePtr shape, const glm::vec3 &power, bool delta):
		shape(shape), power(power) {}

	HittableType type() const { return HittableType::Light; }
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
	glm::vec3 Le(const glm::vec3 &y, const glm::vec3 &We);

	std::optional<LightLiSample> sampleLi(glm::vec3 x, glm::vec2 u);
	float pdfLi(const glm::vec3 &x, const glm::vec3 &y);
	LightLeSample sampleLe(const std::array<float, 6> &u);
	Ray getRandomRay();

protected:
	HittablePtr shape;
	glm::vec3 power;
};

typedef std::shared_ptr<Light> LightPtr;