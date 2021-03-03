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
		glm::vec3 bsdf = this->bsdf(si, sample.type.type());
		return SampleWithBsdf(sample, bsdf);
	}

	const BXDF& bxdf() const { return matBxdf; }

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
	BXDF matBxdf;
};

#endif
