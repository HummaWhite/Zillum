#pragma once

#include "Core/Math.h"
#include "Core/Transform.h"

struct PhaseSample {
	PhaseSample(Vec3f w, float pdf) : wi(w), pdf(pdf), p(pdf) {}
	float p;
	Vec3f wi;
	float pdf;
};

float HGPhaseFunction(float cosTheta, float g);
float HGPhasePDF(Vec3f wo, Vec3f wi, float g);
PhaseSample HGPhaseSample(Vec3f wo, float g, Vec2f u);