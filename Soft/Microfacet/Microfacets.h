#pragma once

#include "Microfacet.h"
#include "DistribTerm.h"
#include "../Math/Transform.h"

class GGXDistrib:
	public MicrofacetDistrib
{
public:
	GGXDistrib(float roughness, bool sampleVisible, float aniso = 0.0f);

	float d(const glm::vec3 &N, const glm::vec3 &M);
	float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo);
	glm::vec3 sampleWm(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec2 &u);
	float g(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec3 &Wi);

private:
	bool visible;
	float alpha;
};

class GTR1Distrib:
	public MicrofacetDistrib
{
public:
	GTR1Distrib(float roughness) : alpha(roughness * roughness) {}

	float d(const glm::vec3 &N, const glm::vec3 &M);
	float pdf(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo);
	glm::vec3 sampleWm(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec2 &u);

private:
	float alpha;
};
