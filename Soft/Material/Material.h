#ifndef MATERIAL_H
#define MATERIAL_H

#include <iostream>
#include <random>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Hit/SurfaceInteraction.h"
#include "../Ray.h"
#include "../BXDF/BXDF.h"
#include "../Math/Transform.h"

struct Sample
{
	Sample(const glm::vec4 &sample, uint8_t param):
		dir(sample), pdf(sample.w), param(param) {}

	Sample(const glm::vec3 &dir, float pdf, uint8_t param):
		dir(dir), pdf(pdf), param(param) {}

	glm::vec3 dir;
	float pdf;
	uint8_t param;
};

class Material
{
public:
	Material(int bxdfType): matBXDF(bxdfType) {}

	virtual glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param) = 0;
	virtual Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo) = 0;

	virtual glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		auto sample = getSample(N, Wi);
		return glm::vec4(sample.dir, sample.pdf);
	}

	virtual float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N) = 0;

	const BXDF& bxdf() const { return matBXDF; }

protected:
	inline static bool refract(glm::vec3& Wt, const glm::vec3& Wi, const glm::vec3 &N, float eta)
	{
		// 与PBRT不同，这个的结果只与光路上折射率的比值eta有关，与N的取向无关
		float cosTi = glm::dot(N, Wi);
		float sin2Ti = 1.0f - cosTi * cosTi;
		float sin2Tt = eta * eta * sin2Ti;

		if (sin2Tt >= 1.0f) return false;

		float dirN = cosTi < 0 ? -1.0f : 1.0f;
		float cosTt = glm::sqrt(1.0f - sin2Tt) * dirN;
		Wt = glm::normalize(-Wi * eta + N * (eta * cosTi - cosTt));
		return true;
	}

	inline static float fresnelDieletric(float cosTi, float etaI, float etaT)
	{
		cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
		float sinTi = glm::sqrt(1 - cosTi * cosTi);
		float sinTt = etaI / etaT * sinTi;
		if (sinTt >= 1.0f) return 1.0f;
		
		float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

		float rPa = (etaT * cosTi - etaI * cosTt) / (etaT * cosTi + etaI * cosTt);
		float rPe = (etaI * cosTi - etaT * cosTt) / (etaI * cosTi + etaT * cosTt);

		float res = (rPa * rPa + rPe * rPe) * 0.5f;
		return res;
	}

	inline static glm::vec2 sqrtAlpha(float roughness, float aniso)
	{
		float r2 = roughness * roughness;
		float al = glm::sqrt(1.0f - aniso * 0.9f);
		return glm::vec2(r2 / al, r2 * al);
	}

	inline static glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0f) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	inline static glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0f - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	float schlickG(float cosTheta, float alpha)
	{
		float k = alpha * 0.5f;
		return cosTheta / (cosTheta * (1.0f - k) + k);
	}

	float smithG(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec3 &Wi, float alpha)
	{
		float NoWi = Math::satDot(N, Wo);
		float NoWo = Math::satDot(N, Wi);

		float g1 = schlickG(NoWo, alpha);
		float g2 = schlickG(NoWi, alpha);

		return g1 * g2;
	}

	float GGX(float cosTheta, float alpha)
	{
		if (cosTheta < 1e-6f) return 0.0f;

		float a2 = alpha * alpha;

		float nom = a2;
		float denom = cosTheta * cosTheta * (a2 - 1.0f) + 1.0f;
		denom = denom * denom * Math::Pi;
		return nom / denom;
	}

	float GGX(float cosTheta, float sinPhi, const glm::vec2 &alpha)
	{
		if (cosTheta < 1e-6f) return 0.0f;

		float sinPhi2 = sinPhi * sinPhi;

		float p = (1.0f - sinPhi2) / (alpha.x * alpha.x) + sinPhi2 / (alpha.y * alpha.y);
		float k = 1.0f + (p - 1.0f) * (1.0f - cosTheta * cosTheta);
		k = k * k * Math::Pi * alpha.x * alpha.y;

		return 1.0f / k;
	}

	float GGXVNDF(float cosTheta, float NoWo, float MoWo, float alpha)
	{
		return schlickG(NoWo, alpha) * GGX(cosTheta, alpha) * MoWo / NoWo;
	}

protected:
	BXDF matBXDF;
};

#endif
