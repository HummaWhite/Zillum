#ifndef SAMPLERS_H
#define SAMPLERS_H

#include <memory>

#include "Sampler.h"
#include "../Light/Lights.h"

class RandomSampler:
	public Sampler
{
public:
	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f));

		glm::vec3 randomVec(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

		glm::vec3 U = (glm::abs(N.z) > 0.98f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 R = glm::normalize(glm::cross(N, U));
		U = glm::normalize(glm::cross(R, N));

		glm::mat3 normMatrix(U, R, N);

		return glm::normalize(normMatrix * randomVec);
	}
};

/*
class CosineSampler:
	public Sampler
{
public:
	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
	}
};
*/

class LightSampler:
	public Sampler
{
public:
	LightSampler(std::shared_ptr<Light> _light):
		light(_light) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return glm::normalize(light->getRandomPoint() - hitPoint);
	}

private:
	std::shared_ptr<Light> light;
};

class RoughnessSampler:
	public Sampler
{
public:
	RoughnessSampler(float _roughness):
		roughness(_roughness) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		glm::vec3 V = Wo;
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(90.0f * roughness));

		glm::vec3 randomVec(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

		glm::vec3 U = (glm::abs(N.z) > 0.98f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 R = glm::normalize(glm::cross(N, U));
		U = glm::normalize(glm::cross(R, N));

		glm::mat3 normMatrix(U, R, N);
		glm::vec3 biasedNorm = glm::normalize(normMatrix * randomVec);

		return glm::reflect(-Wo, biasedNorm);
	}

private:
	float roughness;
};

#endif
