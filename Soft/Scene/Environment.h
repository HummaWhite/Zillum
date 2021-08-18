#pragma once

#include <iostream>
#include <memory>
#include <array>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "Light.h"
#include "../Scene/Ray.h"
#include "../Math/Transform.h"
#include "../Math/RandomGenerator.h"

struct EnvLiSample
{
	glm::vec3 Wi;
	glm::vec3 Li;
	float pdf;
};

class Environment
{
public:
	virtual glm::vec3 getRadiance(const glm::vec3 &dir) = 0;
	virtual EnvLiSample sampleLi(const glm::vec2 &u1, const glm::vec2 &u2) = 0;
	virtual float pdfLi(const glm::vec3 &Wi) = 0;
	virtual LightLeSample sampleLe(float radius, const std::array<float, 6> &u) = 0;
	virtual float power() = 0;
};

typedef std::shared_ptr<Environment> EnvPtr;