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

	virtual glm::vec4 importanceSample()
	{
		RandomGenerator rg;
		return glm::vec4(Transform::planeToSphere(glm::vec2(rg.get(), rg.get())), Math::PiInv * 0.25f);
	}
};

#endif
