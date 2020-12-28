#ifndef HEMISPHERE_SAMPLING_H
#define HEMISPHERE_SAMPLING_H

#include "Math/Math.h"

struct Sample
{
	Sample(const glm::vec4 &sample, uint8_t param):
		dir(sample), pdf(sample.w), param(param) {}

	glm::vec3 dir;
	float pdf;
	uint8_t param;
};

namespace HemisphereSampling
{
	inline static glm::vec4 random(const glm::vec3 &N)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f));
		glm::vec3 randomVec(glm::cos(theta) * glm::sin(phi), glm::sin(theta) * glm::sin(phi), glm::cos(phi));

		return glm::vec4(glm::normalize(Math::TBNMatrix(N) * randomVec), 0.5f / glm::pi<float>());
	}

	inline static glm::vec4 cosineWeighted(const glm::vec3 &N)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f));
		glm::vec3 randomVec(glm::cos(theta) * glm::sin(phi), glm::sin(theta) * glm::sin(phi), glm::cos(phi));

		return glm::vec4(glm::normalize(Math::TBNMatrix(N) * randomVec), glm::cos(phi) / glm::pi<float>());
	}

	inline static glm::vec4 GGX(const glm::vec2 &xi, const glm::vec3 &N, const glm::vec3 &Wo, float roughness)
	{
		float r4 = glm::pow(roughness, 4.0f);
		float phi = 2.0 * glm::pi<float>() * xi.x;
		float sinTheta, cosTheta;

		if (xi.y > 0.99999f)
		{
			cosTheta = 0.0f;
			sinTheta = 1.0f;
		}
		else
		{
			cosTheta = glm::sqrt((1.0f - xi.y) / (1.0f + (r4 - 1.0f) * xi.y));
			sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);
		}

		glm::vec3 H(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta);
		H = glm::normalize(Math::TBNMatrix(N) * H);
		glm::vec3 L = glm::normalize(H * 2.0f * glm::dot(Wo, H) - Wo);
		glm::vec3 V = Wo;

        float NdotH = glm::max(glm::dot(N, H), 0.0f);
        float HdotV = glm::max(glm::dot(H, V), 0.0f);

        float D = Math::distributionGGX(N, H, roughness);
		float pdf = D * NdotH / (4.0 * HdotV + 1e-8f);

		if (HdotV < 1e-6f) pdf = 0.0f;
		if (pdf < 1e-10f) pdf = 0.0f;
		return glm::vec4(L, pdf);
	}

	inline static glm::vec4 GGX(const glm::vec3 &N, const glm::vec3 &Wo, float roughness)
	{
		RandomGenerator rg;
		glm::vec2 xi(rg.get(0.0f, 1.0f), rg.get(0.0f, 1.0f));
		return GGX(xi, N, Wo, roughness);
	}
}

#endif
