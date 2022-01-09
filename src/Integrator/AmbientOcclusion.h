#pragma once

#include "Integrator.h"

class AOIntegrator:
	public PixelIndependentIntegrator
{
public:
	AOIntegrator(ScenePtr scene, int maxSpp):
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AO) {}
	Vec3f tracePixel(Ray ray, SamplerPtr sampler);

private:
	Vec3f trace(Ray ray, Vec3f N, SamplerPtr sampler);

public:
	Vec3f occlusionRadius = Vec3f(1.0f);
	int samples = 1;
};
