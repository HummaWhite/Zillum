#pragma once

#include <iostream>
#include <memory>

#include "Shapes.h"

struct LightSample
{
	glm::vec3 Wi;
	glm::vec3 weight;
	float pdf;
};

class Light:
	public Hittable
{
public:
	Light(std::shared_ptr<Hittable> shape, const glm::vec3 &power, bool delta):
		shape(shape), power(power) {}

	HittableType type() { return HittableType::Light; }

	HitInfo closestHit(const Ray &ray)
	{
		return shape->closestHit(ray);
	}

	glm::vec3 getRandomPoint()
	{
		return shape->getRandomPoint();
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

	void setTransform(std::shared_ptr<Transform> trans) override
	{
		transform = trans;
		shape->setTransform(trans);
	}

	AABB bound()
	{
		return shape->bound();
	}

	glm::vec3 getPower(){ return power; }

	float getRgbPower() { return Math::rgbBrightness(power); }

	glm::vec3 getRadiance(const glm::vec3 &y, const glm::vec3 &Wi)
	{
		return power / (2.0f * Math::Pi * surfaceArea());
	}

	float pdfLi(const glm::vec3 &x, const glm::vec3 &y)
	{
		auto N = surfaceNormal(y);
		auto Wi = glm::normalize(y - x);
		float cosTheta = Math::satDot(N, -Wi);
		if (cosTheta < 1e-10f)
			return 0.0f;

		return Math::distSquare(x, y) / (surfaceArea() * cosTheta);
	}

	Ray getRandomRay()
	{
		glm::vec3 ori = getRandomPoint();
		glm::vec3 N = surfaceNormal(ori);
		glm::vec3 dir = Transform::normalToWorld(N, Math::randHemisphere());
		return { ori + dir * 1e-4f, dir };
	}

	AABB bound() const
	{
		return shape->bound();
	}

protected:
	std::shared_ptr<Hittable> shape;
	glm::vec3 power;
};
