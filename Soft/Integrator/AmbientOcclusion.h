#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "Integrator.h"

class AOIntegrator:
	public PixelIndependentIntegrator
{
public:
	AOIntegrator(int width, int height, int maxSpp):
		PixelIndependentIntegrator(width, height, maxSpp) {}

	glm::vec3 tracePixel(Ray ray)
	{
		auto [dist, obj] = scene->closestHit(ray);
		if (obj == nullptr) return glm::vec3(1.0f);

		auto p = ray.get(dist);
		ray.ori = p;
		return trace(ray, obj->surfaceNormal(p));
	}

private:
	glm::vec3 trace(Ray ray, glm::vec3 N)
	{
		glm::vec3 ao(0.0f);

		for (int i = 0; i < samples; i++)
		{
			auto Wi = Transform::normalToWorld(N, Math::randHemisphere());

			Ray newRay(ray.ori + Wi * 1e-4f, Wi);
			//auto [dist, obj] = scene->closestHit(newRay);

			if (scene->quickIntersect(newRay, occlusionRadius.x)) ao += glm::vec3(1.0f);

			// if (obj == nullptr) continue;

			// if (dist < occlusionRadius.x) ao.x += 1.0f;
			// if (dist < occlusionRadius.y) ao.y += 1.0f;
			// if (dist < occlusionRadius.z) ao.z += 1.0f;
		}

		return glm::vec3(1.0f) - ao / (float)samples;
	}

public:
	glm::vec3 occlusionRadius = glm::vec3(1.0f);
	int samples = 40;
};

#endif
