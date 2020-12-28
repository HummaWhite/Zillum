#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <thread>
#include <mutex>

#include "../FrameBuffer.h"
#include "../Color.h"
#include "../Camera.h"
#include "../Texture.h"
#include "../Hittable/HittableShapes.h"
#include "../Shape/Shapes.h"
#include "../Light/Lights.h"
#include "../Material/Materials.h"
#include "../Math/Math.h"
#include "../Environment/Environments.h"
#include "../Accelerator/BVH.h"
#include "../ObjReader.h"
#include "../Scene.h"

class PixelIndependentIntegrator
{
public:
	PixelIndependentIntegrator(int width, int height, int maxSpp):
		width(width), height(height), maxSpp(maxSpp), maxThreads(std::thread::hardware_concurrency())
	{
		resultBuffer.init(width, height);
		resultBuffer.fill(glm::vec3(0.0f));
	}

	void setScene(std::shared_ptr<Scene> scene) { this->scene = scene; }

	FrameBuffer<glm::vec3>& result() { return resultBuffer; }

	void resizeBuffer(int w, int h)
	{
		width = w, height = h;
		resultBuffer.resize(w, h);
		modified = true;
	}

	void render()
	{
		if (modified)
		{
			resultBuffer.fill(glm::vec3(0.0f));
			curSpp = 0;
			modified = false;
		}
		if ((lowDiscrepSeries || limitSpp) && curSpp >= maxSpp) return;

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

	virtual glm::vec3 tracePixel(Ray ray) = 0;

private:
	void doTracing(int start, int end)
	{
		float invW = 1.0f / width;
		float invH = 1.0f / height;
		for (int x = start; x < end; x++)
		{
			for (int y = 0; y < height; y++)
			{
				float sx = 2.0f * (x + 0.5f) * invW - 1.0f;
				float sy = 1.0f - 2.0f * (y + 0.5f) * invH;

				RandomGenerator rg;
				glm::vec2 uniSample = lowDiscrepSeries ? Math::hammersley(curSpp, maxSpp) : glm::vec2(rg.get(), rg.get());
				glm::vec2 sp = uniSample - 0.5f;

				Ray ray = scene->camera->getRay(sx + sp.x * invW, sy + sp.y * invH);
				glm::vec3 result = tracePixel(ray);

				if (Math::isNan(result.x) || Math::isNan(result.y) || Math::isNan(result.z))
				{
					std::cout << "Ooops! nan occurred!\n";
					result = glm::vec3(0.0f);
				}

				result = glm::clamp(result, glm::vec3(0.0f), glm::vec3(1e8f));
				resultBuffer(x, y) = resultBuffer(x, y) * ((float)(curSpp) / (float)(curSpp + 1)) + result / (float)(curSpp + 1);
			}
		}
	}

public:
	bool modified = false;
	bool lowDiscrepSeries = false;
	bool limitSpp = false;

protected:
	const int maxThreads;
	const int maxSpp;
	int curSpp = 0;

	int width, height;

	std::shared_ptr<Scene> scene;

private:
	FrameBuffer<glm::vec3> resultBuffer;
};

#endif
