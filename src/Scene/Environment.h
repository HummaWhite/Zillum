#pragma once

#include <iostream>
#include <memory>
#include <array>

#include "../glm/glmIncluder.h"

#include "Light.h"
#include "../Scene/Ray.h"
#include "../Math/Transform.h"
#include "../Math/RandomGenerator.h"

struct EnvLiSample
{
	Vec3f Wi;
	Vec3f Li;
	float pdf;
};

class Environment
{
public:
	virtual Vec3f getRadiance(const Vec3f &dir) = 0;
	virtual EnvLiSample sampleLi(const Vec2f &u1, const Vec2f &u2) = 0;
	virtual float pdfLi(const Vec3f &Wi) = 0;
	virtual LightLeSample sampleLe(float radius, const std::array<float, 6> &u) = 0;
	virtual float power() = 0;
};

using EnvPtr = std::shared_ptr<Environment>;