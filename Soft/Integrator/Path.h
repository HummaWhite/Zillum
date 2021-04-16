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
			return lt->getRadiance(ray.get(dist), ray.dir);
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
	glm::vec3 trace(Ray ray, SurfaceInfo sInfo)
	{
		glm::vec3 result(0.0f);
		glm::vec3 beta(1.0f);

		for (int bounce = 1; bounce < tracingDepth; bounce++)
		{
			glm::vec3 P = ray.ori;
			glm::vec3 Wo = -ray.dir;
			glm::vec3 N = sInfo.norm;
			auto &mat = sInfo.material;

			bool deltaBsdf = sInfo.material->bxdf().isDelta();

			if (!deltaBsdf && sampleDirectLight)
			{
				auto [Wi, coef, samplePdf] = scene->sampleLightSource(P);
				float bsdfPdf = mat->pdf(Wo, Wi, N);
				float weight = Math::biHeuristic(samplePdf, bsdfPdf);
				result += mat->bsdf({ Wo, Wi, N }, 0) * beta * Math::satDot(N, Wi) * coef * weight;
			}

			auto [sample, bsdf] = sInfo.material->sampleWithBsdf(N, Wo);
			auto [Wi, bsdfPdf, type] = sample;

			float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
			if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf)) break;
			beta *= bsdf * NoWi / bsdfPdf;

			Ray newRay(P + Wi * 1e-4f, Wi);
			auto [dist, obj] = scene->closestHit(newRay);

			if (obj == nullptr)
			{
				float weight = 1.0f;
				if (!deltaBsdf)
				{
					float envPdf = scene->env->pdfLi(Wi) * scene->env->power() / scene->lightSourceTotalPower();
					weight = Math::biHeuristic(bsdfPdf, envPdf);
				}
				result += scene->env->getRadiance(Wi) * envStrength * beta * weight;
				break;
			}

			if (obj->type() == HittableType::Light)
			{
				float weight = 1.0f;
				auto lt = dynamic_cast<Light*>(obj.get());
				auto hitPoint = newRay.get(dist);
				if (!deltaBsdf)
				{
					float lightPdf = lt->pdfLi(P, hitPoint) * lt->getRgbPower() / scene->lightSourceTotalPower();
					weight = Math::biHeuristic(bsdfPdf, lightPdf);
				}
				result += lt->getRadiance(hitPoint, -Wi) * beta * weight;
				break;
			}

			if (roulette && uniformFloat() > rouletteProb) break;
			if (roulette) beta /= rouletteProb;
			
			glm::vec3 nextP = newRay.get(dist);
			auto ob = dynamic_cast<Object*>(obj.get());
			sInfo = ob->surfaceInfo(nextP);
			newRay.ori = nextP;
			ray = newRay;
		}
		return result;
	}

public:
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int directLightSample = 1;
	float indirectClamp = 20.0f;
	float envStrength = 1.0f;
	bool sampleDirectLight = false;
};
