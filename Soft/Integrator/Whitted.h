#ifndef WHITTED_H
#define WHITTED_H

#include "Integrator.h"

class WhittedIntegrator:
	public Integrator
{
public:
	WhittedIntegrator(int width, int height, int maxSpp):
		Integrator(width, height, maxSpp) {}

	void render()
	{
		if (modified)
		{
			resultBuffer.fill(glm::vec3(0.0f));
			curSpp = 0;
			modified = false;
		}
		if (lowDiscrepSeries && curSpp >= maxSpp) return;

		std::thread threads[maxThreads];
		for (int i = 0; i < maxThreads; i++)
		{
			int start = (width / maxThreads) * i;
			int end = std::min(width, (width / maxThreads) * (i + 1));
			if (i == maxThreads - 1) end = width;

			threads[i] = std::thread(doTracing, this, start, end);
		}

		for (auto &t : threads) t.join();

		curSpp++;
		std::cout << "\r" << std::setw(4) << curSpp << "/" << maxSpp << " spp  ";

		float perc = (float)curSpp / (float)maxSpp * 100.0f;
		std::cout << "  " << std::fixed << std::setprecision(2) << perc << "%";
	}

private:
	void doTracing(int start, int end)
	{
		for (int x = start; x < end; x++)
		{
			for (int y = 0; y < height; y++)
			{
				float sx = 2.0f * (float)x / width - 1.0f;
				float sy = 1.0f - 2.0f * (float)y / height;

				float sx1 = 2.0f * (float)(x + 1) / width - 1.0f;
				float sy1 = 1.0f - 2.0f * (float)(y + 1) / height;

				RandomGenerator rg;
				glm::vec2 sample = lowDiscrepSeries ? Math::hammersley(curSpp, maxSpp) : glm::vec2(rg.get(0.0f, 1.0f), rg.get(0.0f, 1.0f));
				float sampleX = Math::lerp(sx, sx1, sample.x);
				float sampleY = Math::lerp(sy, sy1, sample.y);
				Ray ray = getRay(sampleX, sampleY);

				glm::vec3 radiance(0.0f);
				auto scHitInfo = scene->closestHit(ray);

				if(scHitInfo.type == SceneHitInfo::LIGHT) radiance = scHitInfo.light->getRadiance();
				else if (scHitInfo.type == SceneHitInfo::SHAPE)
				{
					glm::vec3 hitPoint = ray.get(scHitInfo.dist);
					SurfaceInfo sInfo = scHitInfo.shape->surfaceInfo(hitPoint);
					ray.ori = hitPoint;
					radiance = trace(ray, sInfo, tracingDepth);
				}
				else radiance = scene->environment->getRadiance(ray.dir);

				if (Math::isNan(radiance.x) || Math::isNan(radiance.y) || Math::isNan(radiance.z))
				{
					std::cout << "Ooops! nan occurred!\n";
					radiance = glm::vec3(0.0f);
				}

				radiance = glm::clamp(radiance, glm::vec3(0.0f), glm::vec3(1e8f));
				resultBuffer(x, y) = resultBuffer(x, y) * ((float)(curSpp) / (float)(curSpp + 1)) + radiance / (float)(curSpp + 1);
			}
		}
	}

	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo, int depth)
	{
		glm::vec3 endRadiance = returnEnvColorAtEnd ? scene->environment->getRadiance(ray.dir) : glm::vec3(0.0f);
		if (!roulette && depth == 0) return endRadiance;

		glm::vec3 hitPoint = ray.ori;
		glm::vec3 Wo = -ray.dir;
		glm::vec3 N = surfaceInfo.norm;

		glm::vec3 directRadiance(0.0f);

		if (surfaceInfo.material->bxdf().hasType(BXDF::REFLECTION))
		{
			for (auto &lt : scene->lightList)
			{
				for (int i = 0; i < sampleDirectLight; i++)
				{
					glm::vec3 randomPoint = lt->getRandomPoint();
					glm::vec3 rayDir = glm::normalize(randomPoint - hitPoint);
					float lightDist = glm::length(randomPoint - hitPoint);

					Ray lightRay(hitPoint + rayDir * 0.0001f, rayDir);

					float tMin, tMax;
					auto occShape = scene->shapeBVH->closestHit(lightRay, tMin, tMax);
					if (occShape != nullptr && tMin < lightDist) continue;

					glm::vec3 Wi = rayDir;
					glm::vec3 lightN = lt->surfaceNormal(randomPoint);
					glm::vec3 lightRad = lt->getRadiance(-Wi, lightN, lightDist);
					SurfaceInteraction si = { Wo, Wi, N };
					glm::vec3 outRad = surfaceInfo.material->outRadiance(si, lightRad);

					directRadiance += outRad;
				}
			}
		}

		directRadiance /= (float)sampleDirectLight;

		RandomGenerator rg;
		if (roulette && rg.get(0.0f, 1.0f) > rouletteProb) return directRadiance;

		glm::vec4 sample = surfaceInfo.material->getSample(hitPoint, N, Wo);
		glm::vec3 Wi(sample);
		float pdf = sample.w;
		if (roulette) pdf *= rouletteProb;

		if (pdf == 0.0f || pdf < 1e-8f) return directRadiance;

		Ray newRay(hitPoint + Wi * 0.0001f, Wi);
		glm::vec3 nextRadiance(0.0f);
		auto scHitInfo = scene->closestHit(newRay);

		if (scHitInfo.type == SceneHitInfo::LIGHT)
		{
			glm::vec3 lightNorm = scHitInfo.light->surfaceNormal(newRay.get(scHitInfo.dist));
			nextRadiance = scHitInfo.light->getRadiance(-Wi, lightNorm, scHitInfo.dist);
		}
		else if (scHitInfo.type == SceneHitInfo::SHAPE)
		{
			glm::vec3 nextHitPoint = newRay.get(scHitInfo.dist);
			SurfaceInfo nextSInfo = scHitInfo.shape->surfaceInfo(nextHitPoint);
			newRay.ori = nextHitPoint;
			nextRadiance = trace(newRay, nextSInfo, depth - 1);
		}
		else nextRadiance = scene->environment->getRadiance(Wi);

		SurfaceInteraction si = { Wo, Wi, N };
		glm::vec3 indirectRadiance = surfaceInfo.material->outRadiance(si, nextRadiance) / pdf;
		indirectRadiance = glm::clamp(indirectRadiance, 0.0f, indirectClamp);
		return directRadiance + indirectRadiance;
	}

public:
	bool lowDiscrepSeries = false;
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int sampleDirectLight = 2;
	bool returnEnvColorAtEnd = false;
	float indirectClamp = 20.0f;
};

#endif
