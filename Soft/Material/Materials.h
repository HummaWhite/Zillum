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

	glm::vec3 reflectionRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		glm::vec3 H = glm::normalize(si.Wi + si.Wo);
		glm::vec3 F = Math::fresnelSchlickRoughness(std::max(glm::dot(H, si.Wo), 0.0f), reflRate, roughness);

		return radiance * F * std::max(0.0f, glm::dot(si.N, si.Wi));
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		return sampler.getSample(hitPoint, N, Wo, pdf);
	}

private:
	glm::vec3 reflRate;
	float roughness;

	GlossySampler sampler;
};

class MaterialPBR:
	public Material
{
public:
	MaterialPBR(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness)
	{
		diffuseSampler = std::make_shared<RandomSampler>();
		specularSampler = std::make_shared<GlossySampler>(_roughness);
	}

	glm::vec3 reflectionRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		return reflectionRadiance(si.Wo, si.Wi, si.N, radiance, false);
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		RandomGenerator rg;
		float rd = rg.get(0.0f, 1.0f);
		bool sampleDiffuse = rd < 0.5f * (1.0f - metallic);

		std::shared_ptr<Sampler> sampler = sampleDiffuse ? diffuseSampler : specularSampler;
		glm::vec3 sample = sampler->getSample(hitPoint, N, Wo, pdf);

		return sample;
	}

	glm::vec3 reflectionRadiance(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N, const glm::vec3 &radiance, bool IBL)
	{
		using namespace Math;

		glm::vec3 L = Wi;
		glm::vec3 V = Wo;
		glm::vec3 H = glm::normalize(V + L);

		float NdotL = glm::max(glm::dot(N, L), 1e-6f);
		float NdotV = glm::max(glm::dot(N, V), 1e-6f);

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
		float	  D = distributionGGX(N, H, roughness);
		float	  G = geometrySmith(N, V, L, roughness, IBL);

		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0f) - kS;
		kD *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denominator = 4.0f * NdotV * NdotL + 1e-6f;
		glm::vec3 specular = FDG / denominator;

		return (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;

	std::shared_ptr<Sampler> diffuseSampler;
	std::shared_ptr<Sampler> specularSampler;
};

#endif
