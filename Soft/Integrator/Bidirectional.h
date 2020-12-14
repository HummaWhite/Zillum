#ifndef BIDIRECTIONAL_H
#define BIDIRECTIONAL_H

#include <list>

#include "Integrator.h"

class BidirectionalIntegrator:
	public Integrator
{
public:
	BidirectionalIntegrator(int width, int height, int maxSpp):
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

		/*
		auto lightNodes = genLightNodes();
		for (auto &i : lightNodes)
		{
			Math::printVec3(i.ray.ori, "Ro");
			Math::printVec3(i.ray.dir, "Rd");
			Math::printVec3(i.sInfo.norm, "N");
			Math::printVec3(i.Li, "Li");
			std::cout << "\n";
		}
		*/

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
	struct LightNode
	{
		Ray ray;
		SurfaceInfo sInfo;
		glm::vec3 Li;
		float pdf;
	};

private:
	void genLightNodes(Ray ray, SurfaceInfo sInfo, glm::vec3 Li, float pdf, int depth, std::list<LightNode> &lightNodes)
	{
		lightNodes.push_back(LightNode{ ray, sInfo, Li, pdf });
		if (depth == 0) return;

		glm::vec3 P = ray.ori;
		glm::vec3 Wi = -ray.dir;
		glm::vec3 N = sInfo.norm;
		glm::vec4 sample = sInfo.material->getSampleForward(P, N, Wi);
		glm::vec3 Wo(sample);

		Ray newRay(P + Wo * 1e-4f, Wo);
		auto scHitInfo = scene->closestHit(newRay);
		if (scHitInfo.type != SceneHitInfo::SHAPE) return;

		glm::vec3 nextP = newRay.get(scHitInfo.dist);
		SurfaceInteraction si = { Wo, Wi, N };
		glm::vec3 Lo = sInfo.material->outRadiance(si, Li);
		newRay.ori = nextP;
		auto nextSInfo = scHitInfo.shape->surfaceInfo(nextP);

		genLightNodes(newRay, nextSInfo, Lo, pdf * sample.w, depth - 1, lightNodes);
	}

	std::list<LightNode> genLightNodes()
	{
		std::list<LightNode> ret;
		for (auto &lt : scene->lightList)
		{
			for (int i = 0; i < samplesPerLight; i++)
			{
				Ray ray = lt->getRandomRay();
				auto scHitInfo = scene->closestHit(ray);
				if (scHitInfo.type != SceneHitInfo::SHAPE) continue;

				glm::vec3 hitPoint = ray.get(scHitInfo.dist);
				glm::vec3 lightNorm = lt->surfaceNormal(ray.ori);
				auto sInfo = scHitInfo.shape->surfaceInfo(hitPoint);

				glm::vec3 Li = lt->getRadiance(ray.dir, lightNorm, scHitInfo.dist);
				ray.ori = hitPoint;
				genLightNodes(ray, sInfo, Li, 1.0f, lightDepth, ret);
			}
		}
		return ret;
	}

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

					auto lightNodes = genLightNodes();
					radiance = trace(ray, sInfo, eyeDepth, lightNodes);
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

	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo, int depth, const std::list<LightNode> &lightNodes)
	{
		if (depth == 0) return returnEnvColorAtEnd ? scene->environment->getRadiance(ray.dir) : glm::vec3(0.0f);

		glm::vec3 hitPoint = ray.ori;
		glm::vec3 Wo = -ray.dir;
		glm::vec3 N = surfaceInfo.norm;

		glm::vec3 directRadiance(0.0f);
		if (surfaceInfo.material->bxdf().hasType(BXDF::REFLECTION))
		{
			for (auto &lt : scene->lightList)
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

		for (const auto &node : lightNodes)
		{
			glm::vec3 Wi = glm::normalize(node.ray.ori - hitPoint);

			float pdfEye = surfaceInfo.material->pdf(Wo, Wi, N);
			//float pdfLit = node.sInfo.material->pdf(-Wi, -node.ray.dir, node.sInfo.norm);
			float pdf = pdfEye;

			if (pdf < 1e-8f) continue;

			SurfaceInteraction siLight = { -Wi, -node.ray.dir, node.sInfo.norm };
			glm::vec3 Lo = node.sInfo.material->outRadiance(siLight, node.Li);

			Ray linkRay(hitPoint + Wi * 1e-4f, Wi);
			float linkLength = glm::length(linkRay.ori - node.ray.ori);
			auto scHit = scene->closestHit(linkRay);

			if (scHit.type != SceneHitInfo::NONE && scHit.dist < linkLength) continue;

			SurfaceInteraction siEye = { Wo, Wi, N };
			glm::vec3 outRad = surfaceInfo.material->outRadiance(siEye, Lo);
			directRadiance += outRad * pdfEye;
		}

		glm::vec4 sample = surfaceInfo.material->getSample(hitPoint, N, Wo);
		glm::vec3 Wi(sample);
		float pdf = sample.w;

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
			nextRadiance = trace(newRay, nextSInfo, depth - 1, lightNodes);
		}
		else nextRadiance = scene->environment->getRadiance(Wi);

		SurfaceInteraction si = { Wo, Wi, N };
		glm::vec3 indirectRadiance = surfaceInfo.material->outRadiance(si, nextRadiance) / pdf;
		/*
		if (indirectRadiance.x > pdf * 1000.0f)
		{
			auto H = glm::normalize(Wi + Wo);
			std::cout << std::setprecision(6) << indirectRadiance.x << " " << pdf << "\n";
			std::cout << std::setprecision(6) << "NdotL:  " << glm::dot(Wi, N) << "\nNdotV:  " << glm::dot(Wo, N) << "\nL/VdotH:" << glm::dot(Wi, H) << "\nNdotH:  " << glm::dot(N, H) << "\n";
			Math::printVec3(Wo, "Wo");
			Math::printVec3(Wi, "Wi");
			Math::printVec3(N, "N");
			Math::printVec3(H, "H");
			Math::printVec3(hitPoint, "P");
			std::cout << "\n";
		}
		*/
		indirectRadiance = glm::clamp(indirectRadiance, 0.0f, indirectClamp);
		return directRadiance + indirectRadiance;
	}

public:
	bool lowDiscrepSeries = false;
	int eyeDepth = 5;
	int samplesPerLight = 2;
	int lightDepth = 10;
	bool returnEnvColorAtEnd = false;
	float indirectClamp = 20.0f;
};

#endif
