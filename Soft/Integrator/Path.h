#pragma once

#include "Integrator.h"
#include "../Hittable/Light.h"

class PathIntegrator:
	public PixelIndependentIntegrator
{
public:
	PathIntegrator(int width, int height, int maxSpp):
		PixelIndependentIntegrator(width, height, maxSpp) {}

	glm::vec3 tracePixel(Ray ray)
	{
		auto [dist, obj] = scene->closestHit(ray);

		if (obj == nullptr) return scene->env->getRadiance(ray.dir);

		if (obj->type() == HittableType::Light)
		{
			auto lt = dynamic_cast<Light*>(obj.get());
			return lt->getRadiance();
		}
		else
		{
			glm::vec3 p = ray.get(dist);
			auto ob = dynamic_cast<Object*>(obj.get());
			SurfaceInfo sInfo = ob->surfaceInfo(p);
			ray.ori = p;
			return trace(ray, sInfo);
		}
	}

private:
	glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo)
	{
		glm::vec3 result(0.0f);
		glm::vec3 beta(1.0f);

		for (int bounce = 1; ; bounce++)
		{
			if ((roulette && bounce == roletteMaxDepth) || (!roulette && bounce == tracingDepth))
			{
				if (returnEnvColorAtEnd) result += scene->env->getRadiance(ray.dir) * envStrength * beta;
				break;
			}

			glm::vec3 P = ray.ori;
			glm::vec3 Wo = -ray.dir;
			glm::vec3 N = surfaceInfo.norm;
			auto &mat = surfaceInfo.material;

			bool deltaBsdf = surfaceInfo.material->bxdf().isDelta();

			if (!deltaBsdf)
			{
				if (envImportanceSample)
				{
					auto [Wi, coef, envPdf] = sampleEnvironment(P);
					float bsdfPdf = mat->pdf(Wo, Wi, N);
					float weight = Math::heuristic(1, envPdf, 1, bsdfPdf, 2);
					result += mat->bsdf({ Wo, Wi, N }, 0) * beta * Math::satDot(N, Wi) * coef * weight;
				}
			}

			auto [sample, bsdf] = surfaceInfo.material->sampleWithBsdf(N, Wo);
			auto [Wi, bsdfPdf, type] = sample;

			float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
			if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf)) break;
			beta *= bsdf * NoWi / bsdfPdf;

			Ray newRay(P + Wi * 1e-4f, Wi);
			auto [dist, obj] = scene->closestHit(newRay);

			if (obj == nullptr)
			{
				if (deltaBsdf)
				{
					result += scene->env->getRadiance(Wi) * envStrength * beta;
					break;
				}
				float envPdf = scene->env->pdfLi(Wi);
				float weight = Math::heuristic(1, bsdfPdf, 1, envPdf, 2);
				result += scene->env->getRadiance(Wi) * envStrength * beta * weight;
				break;
			}

			if (obj->type() == HittableType::Light)
			{
				//float lightPdf = lightWiPdf(P, newRay.get(scHitInfo.dist), scHitInfo.light);
				//float weight = Math::heuristic(1, bsdfPdf, 1, lightPdf, 2);
				//result += scHitInfo.light->getRadiance() * beta * weight;
				auto lt = dynamic_cast<Light*>(obj.get());
				result += lt->getRadiance() * beta;
				break;
			}

			if (roulette && uniformFloat() > rouletteProb) break;
			if (roulette) beta /= rouletteProb;
			
			glm::vec3 nextP = newRay.get(dist);
			auto ob = dynamic_cast<Object*>(obj.get());
			surfaceInfo = ob->surfaceInfo(nextP);
			newRay.ori = nextP;
			ray = newRay;
		}
		return result;
	}

	std::tuple<glm::vec3, glm::vec3, float> sampleDirectLight(const glm::vec3 &x, std::shared_ptr<Light> lt)
	{
		glm::vec3 y = lt->getRandomPoint();
		glm::vec3 Wi = glm::normalize(y - x);
		float dist = glm::length(y - x);

		glm::vec3 N = lt->surfaceNormal(y);
		glm::vec3 weight(0.0f);

		Ray lightRay(x + Wi * 1e-4f, Wi);
		if (scene->bvh->closestHit(lightRay, dist, true) == nullptr)
		{
			weight = lt->getRadiance() * Math::satDot(N, -Wi) * lt->surfaceArea() / (dist * dist);
		}

		return { Wi, weight, lt->pdfLi(x, y) };
	}

	std::tuple<glm::vec3, glm::vec3, float> sampleEnvironment(glm::vec3 &x)
	{
		auto [Wi, pdf] = scene->env->importanceSample();
		auto rad = scene->env->getRadiance(Wi);
		Ray ray(x + Wi * 1e-4f, Wi);
		float tmp = 1e6;
		if (scene->bvh->closestHit(ray, tmp, true) != nullptr)
		{
			rad = glm::vec3(0.0f);
			pdf = 1.0f;
		}
		return { Wi, rad / pdf, pdf };
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
