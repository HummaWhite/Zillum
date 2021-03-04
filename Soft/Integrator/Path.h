#ifndef WHITTED_H
#define WHITTED_H

#include "Integrator.h"

class PathIntegrator:
	public PixelIndependentIntegrator
{
public:
	PathIntegrator(int width, int height, int maxSpp):
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
		glm::vec3 beta(1.0f);

		for (int bounce = 1; ; bounce++)
		{
			if (((bounce == maxSpp) && (limitSpp || lowDiscrepSeries))
				|| bounce == roletteMaxDepth)
			{
				if (returnEnvColorAtEnd) result += scene->environment->getRadiance(ray.dir) * envStrength * beta;
				break;
			}

			glm::vec3 P = ray.ori;
			glm::vec3 Wo = -ray.dir;
			glm::vec3 N = surfaceInfo.norm;
			auto &mat = surfaceInfo.material;

			if (!surfaceInfo.material->bxdf().isDelta())
			{
				for (auto &lt : scene->lightList)
				{
					auto [Wi, weight, pdf] = sampleDirectLight(P, lt);
					if (pdf == 0) continue;
					result += mat->bsdf({ Wo, Wi, N }, 0) * weight * beta * Math::satDot(N, Wi) / pdf;
				}
			}

			if (envImportanceSample)
			{
				auto [Wi, weight, pdf] = sampleEnvironment(P);
				result += mat->bsdf({ Wo, Wi, N }, 0) * weight * beta * Math::satDot(N, Wi) / pdf;
			}

			auto [sample, bsdf] = surfaceInfo.material->sampleWithBsdf(N, Wo);
			auto [Wi, bsdfPdf, type] = sample;

			float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
			if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf)) break;

			Ray newRay(P + Wi * 1e-4f, Wi);
			auto scHitInfo = scene->closestHit(newRay);

			if (scHitInfo.type == SceneHitInfo::LIGHT)
			{
				float lightPdf = lightWiPdf(P, newRay.get(scHitInfo.dist), scHitInfo.light);
				float weight = Math::heuristic(1, bsdfPdf, 1, lightPdf, 2);
				result += scHitInfo.light->getRadiance() * beta * bsdf * NoWi * weight;
				break;
			}
			else if (scHitInfo.type == SceneHitInfo::NONE)
			{
				float envPdf = scene->environment->Environment::pdfLi(Wi);
				envPdf = 1.0f;
				float weight = Math::heuristic(1, bsdfPdf, 1, envPdf, 2);
				//result += scene->environment->getRadiance(Wi) * envStrength * beta * bsdf * NoWi * weight;
				break;
			}

			RandomGenerator rg;
			if (roulette && rg.get() > rouletteProb) break;
			beta *= bsdf * NoWi / bsdfPdf;
			if (roulette) beta /= rouletteProb;
			
			glm::vec3 nextP = newRay.get(scHitInfo.dist);
			surfaceInfo = scHitInfo.shape->surfaceInfo(nextP);
			newRay.ori = nextP;
			ray = newRay;
		}
		return result;
	}

	float lightWiPdf(const glm::vec3 &x, const glm::vec3 &y, std::shared_ptr<Light> lt)
	{
		auto N = lt->surfaceNormal(y);
		auto Wi = glm::normalize(y - x);
		float cosTheta = Math::satDot(N, -Wi);
		if (cosTheta < 1e-10f) return 0.0f;

		float dist = glm::distance(x, y);
		return 1.0f / (lt->surfaceArea() * cosTheta * dist * dist);
	}

	std::tuple<glm::vec3, glm::vec3, float> sampleDirectLight(const glm::vec3 &x, std::shared_ptr<Light> lt)
	{
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		float dist = glm::length(y - x);

		glm::vec3 N = lt->surfaceNormal(y);
		glm::vec3 weight = lt->getRadiance() * Math::satDot(N, -Wi) / (dist * dist);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		float pdf = (scene->shapeBVH->closestHit(lightRay, dist, true) != nullptr) ? 0.0f : 1.0f / lt->surfaceArea();
		return { Wi, weight, pdf };
	}

	std::tuple<glm::vec3, glm::vec3, float> sampleEnvironment(glm::vec3 &x)
	{
		auto [Wi, pdf] = scene->environment->importanceSample();
		auto rad = scene->environment->getRadiance(Wi);
		Ray ray(x + Wi * 1e-4f, Wi);
		float tmp = 1e6;
		if (scene->shapeBVH->closestHit(ray, tmp, true) != nullptr)
		{
			rad = glm::vec3(0.0f);
			pdf = 1.0f;
		}
		return { Wi, rad, pdf };
	}

public:
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int roletteMaxDepth = 40;
	int directLightSample = 1;
	bool returnEnvColorAtEnd = false;
	float indirectClamp = 20.0f;
	float envStrength = 1.0f;
	bool envImportanceSample = false;
};

#endif
