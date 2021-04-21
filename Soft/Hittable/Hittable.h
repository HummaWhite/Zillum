#pragma once

#include <iostream>
#include <optional>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Ray.h"
#include "../Accelerator/AABB.h"
#include "../Math/Transform.h"

enum class HittableType
{
	Object, Light
};

class Hittable
{
public:
	virtual std::optional<float> closestHit(const Ray &ray) = 0;
	
	virtual glm::vec3 getRandomPoint() = 0;

	virtual glm::vec3 surfaceNormal(const glm::vec3 &p) = 0;

	virtual float surfaceArea() = 0;

	virtual glm::vec2 surfaceUV(const glm::vec3 &p) = 0;

	virtual AABB bound() = 0;

	virtual HittableType type() { return HittableType::Object; }

	virtual void setTransform(std::shared_ptr<Transform> trans) { transform = trans; }

	virtual void setTransform(const glm::mat4 &trans)
	{
		transform->set(trans);
	}

	std::shared_ptr<Transform> getTransform() const { return transform; }

protected:
	std::shared_ptr<Transform> transform = std::make_shared<Transform>();
};
