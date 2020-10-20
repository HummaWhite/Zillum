#ifndef SAMPLERS_H
#define SAMPLERS_H

#include <memory>

#include "Sampler.h"
#include "../Light/Lights.h"

class RandomSampler:
	public Sampler
{
public:
	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f));

		glm::vec3 randomVec(glm::cos(theta) * glm::sin(phi), glm::sin(theta) * glm::sin(phi), glm::cos(phi));
		pdf = 0.5f / glm::pi<float>();

		return glm::normalize(Math::TBNMatrix(N) * randomVec);
	}
};

class CosineSampler:
	public Sampler
{
public:
	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f));

		glm::vec3 randomVec(glm::cos(theta) * glm::sin(phi), glm::sin(theta) * glm::sin(phi), glm::cos(phi));
		pdf = glm::sin(phi) * glm::cos(phi) / glm::pi<float>();

		return glm::normalize(Math::TBNMatrix(N) * randomVec);
	}
};

class LightSampler:
	public Sampler
{
public:
	LightSampler(std::shared_ptr<Light> _light):
		light(_light) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		return glm::normalize(light->getRandomPoint() - hitPoint);
	}

private:
	std::shared_ptr<Light> light;
};

class GlossySampler:
	public Sampler
{
public:
	GlossySampler(float _roughness):
		roughness(_roughness) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		glm::vec3 V = Wo;
		RandomGenerator rg;

		glm::vec2 xi(rg.get(0.0f, 1.0f), rg.get(0.0f, 1.0f));

		glm::vec3 H = Math::importanceSampleGGX(xi, N, roughness);
		glm::vec3 L = glm::normalize(H * 2.0f * glm::dot(Wo, H) - Wo);

        float NdotH = glm::max(glm::dot(N, H), 0.0f);
        float HdotV = glm::max(glm::dot(H, V), 0.0f);

        float D = Math::distributionGGX(N, H, roughness);
		pdf = D * NdotH / (4.0 * HdotV) + 1e-6f;

		return L;
	}

private:
	float roughness;
};

#endif
