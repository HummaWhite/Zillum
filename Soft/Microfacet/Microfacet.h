#pragma once

#include "../Math/Math.h"

class MicrofacetDistrib
{
public:
	virtual float d(const Vec3f &N, const Vec3f &M) = 0;
	virtual float pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo) = 0;
	virtual Vec3f sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u) = 0;
};