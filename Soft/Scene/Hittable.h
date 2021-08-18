#pragma once

#include <iostream>
#include <optional>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "Ray.h"
#include "../Accelerator/AABB.h"
#include "../Math/Transform.h"

enum class HittableType
{
	Object, Light, Shape
};

class Hittable
{
public:
	Hittable(HittableType type) : type(type) {}

	virtual std::optional<float> closestHit(const Ray &ray) = 0;
	virtual glm::vec3 uniformSample(const glm::vec2 &u) = 0;
	virtual glm::vec3 surfaceNormal(const glm::vec3 &p) = 0;
	virtual float surfaceArea() = 0;
	virtual glm::vec2 surfaceUV(const glm::vec3 &p) = 0;
	virtual AABB bound() = 0;

	HittableType getType() const { return type; }

	virtual void setTransform(TransformPtr trans) { transform = trans; }
	virtual void setTransform(const glm::mat4 &trans) { transform->set(trans); }
	TransformPtr getTransform() const { return transform; }

protected:
	TransformPtr transform = std::make_shared<Transform>();
	HittableType type;
};

typedef std::shared_ptr<Hittable> HittablePtr;