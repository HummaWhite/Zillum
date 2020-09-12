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
		reflRate(_reflRate), roughness(_roughness) {}

	glm::vec3 reflectionRadiance(const glm::vec3 &Lo, const glm::vec3 &Li, const glm::vec3 &N, const glm::vec3 &radiance)
	{
		RandomGenerator rg;

		float theta = rg.get(0.0f, glm::radians(360.0f));
		float phi = rg.get(0.0f, glm::radians(180.0f));

		glm::vec3 randomVec(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));

		glm::vec3 biasedNorm = glm::normalize(N + randomVec * roughness);

		glm::vec3 H = glm::normalize(Lo - Li);

		return radiance * reflRate * std::max(0.0f, glm::dot(H, Lo));
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Lo)
	{
		return sampler.getSample(hitPoint, N, Lo);
	}

private:
	glm::vec3 reflRate;
	float roughness;

	RandomSampler sampler;
};

class MaterialPBR:
	public Material
{
public:
	MaterialPBR(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness)
	{
		diffuseSampler = std::make_shared<RandomSampler>();
		specularSampler = std::make_shared<IBLImportanceSampler>(_roughness);
	}

	glm::vec3 reflectionRadiance(const glm::vec3 &Lo, const glm::vec3 &Li, const glm::vec3 &N, const glm::vec3 &radiance)
	{
		glm::vec3 L = -Li;
		glm::vec3 V = Lo;
		glm::vec3 H = glm::normalize(V + L);

		//metallic = 1.0f;

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = fresnelSchlickRoughness(std::max(glm::dot(H, V), 0.0f), F0, roughness);
		//glm::vec3 F = fresnelSchlick(std::max(glm::dot(N, V), 0.0f), F0);
		float	  D = distributionGGX(N, H, roughness);
		float	  G = geometrySmith(N, V, L, roughness, false);

		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0f) - kS;
		kD *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denominator = 4.0f * std::max(glm::dot(N, V), 0.0f) * std::max(glm::dot(N, L), 0.0f) + 0.001f;
		glm::vec3 specular = FDG / denominator;

		float NdotL = std::max(glm::dot(N, L), 0.0f);
		return (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Lo)
	{
		//return specularSampler->getSample(hitPoint, N, Lo);
		RandomGenerator rg;

		float rd = rg.get(0.0f, 1.0f);

		if (rd < 0.4f) return diffuseSampler->getSample(hitPoint, N, Lo);
		return specularSampler->getSample(hitPoint, N, Lo);
	}

private:
	glm::vec3 fresnelSchlick(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0) - F0) * (float)glm::pow(1.0 - cosTheta, 5.0);
	}

	glm::vec3 fresnelSchlickRoughness(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0 - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	float distributionGGX(const glm::vec3 &N, const glm::vec3 &H, float roughness)
	{
		float a = roughness * roughness;
		float a2 = a * a;
		float NdotH = std::max(glm::dot(N, H), 0.0f);
		float NdotH2 = NdotH * NdotH;

		float nom = a2;
		float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
		denom = denom * denom * glm::pi<float>();

		return nom / denom;
	}

	float geometrySchlickGGX(float NdotV, float roughness, bool IBL)
	{
		float r = roughness + 1.0f;
		float k = IBL ? roughness * roughness / 2.0f : r * r / 8.0f;

		float nom = NdotV;
		float denom = NdotV * (1.0f - k) + k;

		return nom / denom;
	}

	float geometrySmith(const glm::vec3 &N, const glm::vec3 &V, const glm::vec3 &L, float roughness, bool IBL)
	{
		float NdotV = std::max(glm::dot(N, V), 0.0f);
		float NdotL = std::max(glm::dot(N, L), 0.0f);

		float ggx2 = geometrySchlickGGX(NdotV, roughness, IBL);
		float ggx1 = geometrySchlickGGX(NdotL, roughness, IBL);

		return ggx1 * ggx2;
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;

	std::shared_ptr<Sampler> diffuseSampler;
	std::shared_ptr<Sampler> specularSampler;
};

#endif
