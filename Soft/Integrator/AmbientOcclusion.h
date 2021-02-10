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
		glm::vec3 radiance(0.0f);
		auto scHitInfo = scene->closestHit(ray);

		if (scHitInfo.type == SceneHitInfo::SHAPE)
		{
			glm::vec3 hitPoint = ray.get(scHitInfo.dist);
			SurfaceInfo sInfo = scHitInfo.shape->surfaceInfo(hitPoint);
			ray.ori = hitPoint;
			return trace(ray, sInfo);
		}
		return glm::vec3(1.0f);
	}

private:
	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo)
	{
		glm::vec3 hitPoint = ray.ori;
		glm::vec3 N = surfaceInfo.norm;

		glm::vec3 ao(0.0f);

		for (int i = 0; i < samples; i++)
		{
			auto Wi = Transform::normalToWorld(N, Math::randHemisphere());

			Ray newRay(hitPoint + Wi * 0.0001f, Wi);
			auto scHitInfo = scene->closestHit(newRay);

			if (scHitInfo.type != SceneHitInfo::SHAPE) continue;
			if (scHitInfo.dist < occlusionRadius.x) ao.x += 1.0f;
			if (scHitInfo.dist < occlusionRadius.y) ao.y += 1.0f;
			if (scHitInfo.dist < occlusionRadius.z) ao.z += 1.0f;
		}

		return glm::vec3(1.0f) - ao / (float)samples;
	}

public:
	glm::vec3 occlusionRadius = glm::vec3(1.0f);
	int samples = 40;
};

#endif
