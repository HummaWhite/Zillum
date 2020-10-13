#ifndef MATERIAL_H
#define MATERIAL_H

#include <iostream>
#include <random>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Ray.h"

class Material
{
public:
	virtual glm::vec3 reflectionRadiance(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N, const glm::vec3 &radiance) = 0;
	virtual glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo) = 0;
};

#endif
