#ifndef WHITTED_H
#define WHITTED_H

#include "Integrator.h"

class WhittedIntegrator:
	public PixelIndependentIntegrator
{
public:
	WhittedIntegrator(int width, int height, int maxSpp):
		PixelIndependentIntegrator(width, height, maxSpp) {}

	glm::vec3 tracePixel(Ray ray)
	{
		glm::vec3 radiance(0.0f);
		auto scHitInfo = scene->closestHit(ray);

		if(scHitInfo.type == SceneHitInfo::LIGHT) return scHitInfo.light->getRadiance();
		else if (scHitInfo.type == SceneHitInfo::SHAPE)
		{
			glm::vec3 hitPoint = ray.get(scHitInfo.dist);
			SurfaceInfo sInfo = scHitInfo.shape->surfaceInfo(hitPoint);
			ray.ori = hitPoint;
			return trace(ray, sInfo);
		}
		else return scene->environment->getRadiance(ray.dir);
	}

private:
	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo)
	{
		glm::vec3 result(0.0f);
		glm::vec3 accumBsdf(1.0f);
		float accumPdf = 1.0f;
		glm::vec3 beta(1.0f);

		for (int bounce = 1; ; bounce++)
		{
			if (accumPdf < 1e-8f) break;

			if (((bounce == maxSpp) && (limitSpp || lowDiscrepSeries))
				|| bounce == roletteMaxDepth)
			{
				if (returnEnvColorAtEnd) result += scene->environment->getRadiance(ray.dir) * envStrength * beta;
				break;
			}

			glm::vec3 P = ray.ori;
			glm::vec3 Wo = -ray.dir;
			glm::vec3 N = surfaceInfo.norm;

			for (auto &lt : scene->lightList)
			{
				for (int i = 0; i < sampleDirectLight; i++)
				{
					glm::vec3 randP = lt->getRandomPoint();
					glm::vec3 rayDir = glm::normalize(randP - P);
					float dist = glm::length(randP - P);

					Ray lightRay(P + rayDir * 1e-4f, rayDir);
					float tMin, tMax;
					auto occ = scene->shapeBVH->closestHit(lightRay, tMin, tMax);
					if (occ != nullptr && tMin < dist) continue;

					glm::vec3 Wi = rayDir;
					glm::vec3 lightN = lt->surfaceNormal(randP);
					glm::vec3 lightRad = lt->getRadiance(-Wi, lightN, dist);

					SurfaceInteraction si = { Wo, Wi, N };
					result += surfaceInfo.material->bsdf(si, ~0) * lightRad * beta / (float)sampleDirectLight;
				}
			}

			RandomGenerator rg;
			if (roulette && rg.get() > rouletteProb) break;

			Sample sample = surfaceInfo.material->getSample(P, N, Wo);
			glm::vec3 Wi = sample.dir;
			float pdf = sample.pdf;
			uint8_t param = sample.param;

			if (roulette) pdf *= rouletteProb;
			if (pdf < 1e-8f) break;

			SurfaceInteraction si = { Wo, Wi, N };
			accumBsdf *= surfaceInfo.material->bsdf(si, param);
			accumPdf *= pdf;
			beta = accumBsdf / accumPdf;

			Ray newRay(P + Wi * 1e-4f, Wi);
			auto scHitInfo = scene->closestHit(newRay);

			if (scHitInfo.type == SceneHitInfo::LIGHT)
			{
				glm::vec3 lightN = scHitInfo.light->surfaceNormal(newRay.get(scHitInfo.dist));
				result += scHitInfo.light->getRadiance(-Wi, lightN, scHitInfo.dist) * beta;
				break;
			}
			else if (scHitInfo.type == SceneHitInfo::NONE)
			{
				result += scene->environment->getRadiance(Wi) * envStrength * beta;
				break;
			}
			
			glm::vec3 nextP = newRay.get(scHitInfo.dist);
			surfaceInfo = scHitInfo.shape->surfaceInfo(nextP);
			newRay.ori = nextP;
			ray = newRay;
		}

		return result;
	}

public:
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int roletteMaxDepth = 40;
	int sampleDirectLight = 1;
	bool returnEnvColorAtEnd = false;
	float indirectClamp = 20.0f;
	float envStrength = 1.0f;
};

#endif
