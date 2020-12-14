#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "Integrator.h"

class AOIntegrator:
	public Integrator
{
public:
	AOIntegrator(int width, int height, int maxSpp):
		Integrator(width, height, maxSpp) {}

	inline void render()
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

				glm::vec3 ao(1.0f);
				auto scHitInfo = scene->closestHit(ray);

				if(scHitInfo.type == SceneHitInfo::LIGHT) ;
				else if (scHitInfo.type == SceneHitInfo::SHAPE)
				{
					glm::vec3 hitPoint = ray.get(scHitInfo.dist);
					SurfaceInfo sInfo = scHitInfo.shape->surfaceInfo(hitPoint);
					ray.ori = hitPoint;
					ao = trace(ray, sInfo);
				}

				resultBuffer(x, y) = resultBuffer(x, y) * ((float)(curSpp) / (float)(curSpp + 1)) + ao / (float)(curSpp + 1);
			}
		}
	}

	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo)
	{
		glm::vec3 hitPoint = ray.ori;
		glm::vec3 N = surfaceInfo.norm;

		glm::vec3 ao(0.0f);

		for (int i = 0; i < samples; i++)
		{
			glm::vec3 Wi = HemisphereSampling::random(N);

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
	bool lowDiscrepSeries = false;
	glm::vec3 occlusionRadius = glm::vec3(1.0f);
	int samples = 40;
};

#endif
