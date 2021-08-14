#pragma once

#include "Integrator.h"
#include "../Scene/Light.h"

class PathIntegrator:
	public PixelIndependentIntegrator
{
public:
	PathIntegrator(ScenePtr scene, int maxSpp):
		PixelIndependentIntegrator(scene, maxSpp) {}
	glm::vec3 tracePixel(Ray ray, SamplerPtr sampler);

private:
	glm::vec3 trace(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler);

public:
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int directLightSample = 1;
	float indirectClamp = 20.0f;
	float envStrength = 1.0f;
	bool sampleDirectLight = false;
};
