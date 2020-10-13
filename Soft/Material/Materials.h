#ifndef MATERIALS_H
#define MATERIALS_H

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"
#include "../Sampler/Samplers.h"

class SampleMaterial:
	public Material
{
public:
	SampleMaterial(const glm::vec3 &_reflRate, float _roughness):
		reflRate(_reflRate), roughness(_roughness), sampler(_roughness) {}

	glm::vec3 reflectionRadiance(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N, const glm::vec3 &radiance)
	{
		/*
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(180.0f));

		glm::vec3 randomVec(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
		glm::vec3 biasedNorm = glm::normalize(N + randomVec * roughness);
		*/

		glm::vec3 H = glm::normalize(Wi + Wo);
		glm::vec3 F = Math::fresnelSchlickRoughness(std::max(glm::dot(H, Wo), 0.0f), reflRate, roughness);

		return radiance * F * std::max(0.0f, glm::dot(N, Wi));
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return sampler.getSample(hitPoint, N, Wo);
	}

private:
	glm::vec3 reflRate;
	float roughness;

	RoughnessSampler sampler;
};

class MaterialPBR:
	public Material
{
public:
	MaterialPBR(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness)
	{
		diffuseSampler = std::make_shared<RandomSampler>();
		specularSampler = std::make_shared<RoughnessSampler>(_roughness);
	}

	glm::vec3 reflectionRadiance(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N, const glm::vec3 &radiance)
	{
		return reflectionRadiance(Wo, Wi, N, radiance, false);
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;

		float rd = rg.get(0.0f, 1.0f);

		if (rd < 0.5f * (1.0f - metallic)) return diffuseSampler->getSample(hitPoint, N, Wo);
		return specularSampler->getSample(hitPoint, N, Wo);
	}

	glm::vec3 reflectionRadiance(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N, const glm::vec3 &radiance, bool IBL)
	{
		using namespace Math;

		glm::vec3 L = Wi;
		glm::vec3 V = Wo;
		glm::vec3 H = glm::normalize(V + L);

		float NdotL = std::max(glm::dot(N, L), 0.0f);
		float NdotV = std::max(glm::dot(N, V), 0.0f);

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = fresnelSchlickRoughness(std::max(glm::dot(H, V), 0.0f), F0, roughness);
		float	  D = distributionGGX(N, H, roughness);
		float	  G = geometrySmith(N, V, L, roughness, IBL);

		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0f) - kS;
		kD *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denominator = 4.0f * NdotV * NdotL + 0.001f;
		glm::vec3 specular = FDG / denominator;

		return (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
	}

	glm::vec3 getDiffuseSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return diffuseSampler->getSample(hitPoint, N, Wo);
	}

	glm::vec3 getSpecularSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return specularSampler->getSample(hitPoint, N, Wo);
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;

	std::shared_ptr<Sampler> diffuseSampler;
	std::shared_ptr<Sampler> specularSampler;
};

#endif
