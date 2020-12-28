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
#include "../HemisphereSampling.h"

class Material
{
public:
	Material(int bxdfType): matBXDF(bxdfType) {}

	virtual glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param) = 0;
	virtual Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo) = 0;

	virtual glm::vec4 getSampleForward(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wi)
	{
		auto sample = getSample(hitPoint, N, Wi);
		return glm::vec4(sample.dir, sample.pdf);
	}

	virtual float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N) = 0;

	const BXDF& bxdf() const { return matBXDF; }

protected:
	BXDF matBXDF;
};

#endif
