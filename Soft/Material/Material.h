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
#include "../Microfacet/Microfacets.h"

struct Sample
{
	Sample(): dir(0.0f), pdf(0.0f), param(0) {}

	Sample(const glm::vec4 &sample, uint8_t param):
		dir(sample), pdf(sample.w), param(param) {}

	Sample(const glm::vec3 &dir, float pdf, uint8_t param):
		dir(dir), pdf(pdf), param(param) {}

	glm::vec3 dir;
	float pdf;
	uint8_t param;
};

typedef std::pair<Sample, glm::vec3> SampleWithBsdf;

class Material
{
public:
	Material(int bxdfType): matBXDF(bxdfType) {}

	virtual glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param) = 0;
	virtual Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo) = 0;

	virtual glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		Sample sample = getSample(N, Wi);
		return glm::vec4(sample.dir, sample.pdf);
	}

	virtual float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N) = 0;

	virtual SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		Sample sample = getSample(N, Wo);
		SurfaceInteraction si = { Wo, sample.dir, N };
		glm::vec3 bsdf = this->bsdf(si, sample.param);
		return SampleWithBsdf(sample, bsdf);
	}

	const BXDF& bxdf() const { return matBXDF; }

protected:
	inline static glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0f) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	inline static glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0f - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

protected:
	BXDF matBXDF;
};

#endif
