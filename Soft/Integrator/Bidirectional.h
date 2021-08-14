#pragma once

#include "Integrator.h"

class BidirectionalIntegrator:
	public PixelIndependentIntegrator
{
public:
	BidirectionalIntegrator(int width, int height, int maxSpp):
		PixelIndependentIntegrator(width, height, maxSpp) {}
	glm::vec3 tracePixel(Ray ray);

private:

public:
	int eyeDepth = 5;
	int samplesPerLight = 2;
	int lightDepth = 10;
};
