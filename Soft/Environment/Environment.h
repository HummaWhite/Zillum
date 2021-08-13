#pragma once

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Transform.h"
#include "../Math/RandomGenerator.h"

class Environment
{
public:
	virtual glm::vec3 getRadiance(const glm::vec3 &dir) = 0;

	virtual std::pair<glm::vec3, float> importanceSample(const glm::vec2 &u1, const glm::vec2 &u2)
	{
		return { Transform::planeToSphere(glm::vec2(uniformFloat(), uniformFloat())), Math::PiInv * 0.25f };
	}

	virtual float pdfLi(const glm::vec3 &Wi)
	{
		return Math::PiInv * 0.25f;
	}

	virtual float power() = 0;
};

typedef std::shared_ptr<Environment> EnvironmentPtr;