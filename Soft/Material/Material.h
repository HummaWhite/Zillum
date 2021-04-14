#pragma once

#include <iostream>
#include <random>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "../Surface/SurfaceInteraction.h"
#include "../Ray.h"
#include "../BXDF/BXDF.h"
#include "../Math/Transform.h"
#include "../Microfacet/Microfacets.h"

struct Sample
{
	Sample(): dir(0.0f), pdf(0.0f), type(0) {}

	Sample(const glm::vec4 &sample, int type):
		dir(sample), pdf(sample.w), type(type) {}

	Sample(const glm::vec3 &dir, float pdf, int type):
		dir(dir), pdf(pdf), type(type) {}

	glm::vec3 dir;
	float pdf;
	BXDF type;
};

typedef std::pair<Sample, glm::vec3> SampleWithBsdf;
const SampleWithBsdf INVALID_BSDF_SAMPLE = SampleWithBsdf(Sample(), glm::vec3(0.0f));

class Material
{
public:
	Material(int bxdfType): matBxdf(bxdfType) {}

	virtual glm::vec3 bsdf(const SurfaceInteraction &si, int type) = 0;
	virtual Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo) = 0;
	virtual float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N) = 0;

	virtual SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		Sample sample = getSample(N, Wo);
		SurfaceInteraction si = { Wo, sample.dir, N };
		glm::vec3 bsdf = this->bsdf(si, sample.type.type());
		return SampleWithBsdf(sample, bsdf);
	}

	virtual glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		Sample sample = getSample(N, Wi);
		return glm::vec4(sample.dir, sample.pdf);
	}

	const BXDF& bxdf() const { return matBxdf; }

protected:
	inline static bool refract(glm::vec3 &Wt, const glm::vec3 &Wi, const glm::vec3 &N, float eta)
	{
		float cosTi = glm::dot(N, Wi);
		if (cosTi < 0) eta = 1.0f / eta;
		float sin2Ti = glm::max(0.0f, 1.0f - cosTi * cosTi);
		float sin2Tt = sin2Ti / (eta * eta);

		if (sin2Tt >= 1.0f) return false;

		float cosTt = glm::sqrt(1.0f - sin2Tt);
		if (cosTi < 0) cosTt = -cosTt;
		Wt = glm::normalize(-Wi / eta + N * (cosTi / eta - cosTt));
		return true;
	}

	inline static float fresnelDielectric(float cosTi, float eta)
	{
		cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
		if (cosTi < 0.0f)
		{
			eta = 1.0f / eta;
			cosTi = -cosTi;
		}

		float sinTi = glm::sqrt(1.0f - cosTi * cosTi);
		float sinTt = sinTi / eta;
		if (sinTt >= 1.0f) return 1.0f;

		float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

		float rPa = (cosTi - eta * cosTt) / (cosTi + eta * cosTt);
		float rPe = (eta * cosTi - cosTt) / (eta * cosTi + cosTt);
		return (rPa * rPa + rPe * rPe) * 0.5f;
	}

protected:
	BXDF matBxdf;
};
