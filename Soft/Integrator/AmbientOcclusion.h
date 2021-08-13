#pragma once

#include "Integrator.h"

class AOIntegrator:
	public PixelIndependentIntegrator
{
public:
	AOIntegrator(int width, int height, int maxSpp):
		PixelIndependentIntegrator(width, height, maxSpp) {}
	glm::vec3 tracePixel(Ray ray, SamplerPtr sampler);

private:
	glm::vec3 trace(Ray ray, glm::vec3 N, SamplerPtr sampler);

public:
	glm::vec3 occlusionRadius = glm::vec3(1.0f);
	int samples = 1;
};
