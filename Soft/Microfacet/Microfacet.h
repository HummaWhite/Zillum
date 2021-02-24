#ifndef MICROFACET_H
#define MICROFACET_H

#include "../Math/Math.h"

class MicrofacetDistrib
{
public:
	virtual float d(const glm::vec3 &N, const glm::vec3 &M) = 0;
	virtual float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo) = 0;
	virtual glm::vec3 sampleM(const glm::vec3 &N) = 0;
};

#endif
