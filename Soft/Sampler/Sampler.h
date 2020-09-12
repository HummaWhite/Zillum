#ifndef SAMPLER_H
#define SAMPLER_H

#include <iostream>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Math/RandomGenerator.h"

class Sampler
{
public:
	virtual glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Lo) = 0;
};

#endif
