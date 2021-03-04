#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <iostream>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/Transform.h"
#include "../Math/RandomGenerator.h"

class Environment
{
public:
	virtual glm::vec3 getRadiance(const glm::vec3 &dir) = 0;

	virtual std::pair<glm::vec3, float> importanceSample()
	{
		RandomGenerator rg;
		return { Transform::planeToSphere(glm::vec2(rg.get(), rg.get())), Math::PiInv * 0.25f };
	}

	virtual float pdfLi(const glm::vec3 &Wi)
	{
		return Math::PiInv * 0.25f;
	}
};

#endif
