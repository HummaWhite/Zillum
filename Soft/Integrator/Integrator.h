#pragma once

#include <thread>
#include <mutex>

#include "../FrameBuffer.h"
#include "../Color.h"
#include "../Camera.h"
#include "../Texture.h"
#include "../Hittable/Object.h"
#include "../Hittable/Light.h"
#include "../Material/Materials.h"
#include "../Math/Math.h"
#include "../Environment/Environments.h"
#include "../Accelerator/BVH.h"
#include "../ObjReader.h"
#include "../Scene.h"
#include "../Sampler/Samplers.h"

const int MaxThreads = std::thread::hardware_concurrency();

class PixelIndependentIntegrator
{
public:
	PixelIndependentIntegrator(int width, int height, int maxSpp):
		width(width), height(height), maxSpp(maxSpp)
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
		if (limitSpp && curSpp >= maxSpp) return;

		std::thread threads[MaxThreads];
		for (int i = 0; i < MaxThreads; i++)
		{
			int start = (width / MaxThreads) * i;
			int end = std::min(width, (width / MaxThreads) * (i + 1));
			if (i == MaxThreads - 1) end = width;

			auto threadSampler = (i == 0) ? mSampler : mSampler->copy();
			threads[i] = std::thread(doTracing, this, start, end, threadSampler);
		}
		mSampler->nextSample();

		for (auto &t : threads) t.join();

		curSpp++;
		std::cout << "\r" << std::setw(4) << curSpp << "/" << maxSpp << " spp  ";

		float perc = (float)curSpp / (float)maxSpp * 100.0f;
		std::cout << "  " << std::fixed << std::setprecision(2) << perc << "%";
	}

	virtual glm::vec3 tracePixel(Ray ray, std::shared_ptr<Sampler> sampler) = 0;

private:
	void doTracing(int start, int end, std::shared_ptr<Sampler> sampler)
	{
		float invW = 1.0f / width;
		float invH = 1.0f / height;
		for (int x = start; x < end; x++)
		{
			for (int y = 0; y < height; y++)
			{
				sampler->setPixel(x, y);
				float sx = 2.0f * (x + 0.5f) * invW - 1.0f;
				float sy = 1.0f - 2.0f * (y + 0.5f) * invH;

				glm::vec2 sp = sampler->get2D() - 0.5f;

				Ray ray = scene->camera->getRay(sx + sp.x * invW, sy + sp.y * invH);
				glm::vec3 result = tracePixel(ray, sampler);

				if (Math::isNan(result.x) || Math::isNan(result.y) || Math::isNan(result.z))
				{
					std::cout << "[Computational Error] NAN value occurred\n";
					result = glm::vec3(0.0f);
				}

				result = glm::clamp(result, glm::vec3(0.0f), glm::vec3(1e8f));
				resultBuffer(x, y) = resultBuffer(x, y) * ((float)(curSpp) / (float)(curSpp + 1)) + result / (float)(curSpp + 1);
			}
		}
	}

public:
	bool modified = false;
	bool limitSpp = false;
	std::shared_ptr<Sampler> mSampler;

protected:
	const int maxSpp;
	int curSpp = 0;

	int width, height;

	std::shared_ptr<Scene> scene;

private:
	FrameBuffer<glm::vec3> resultBuffer;
};
