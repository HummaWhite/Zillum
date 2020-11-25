#ifndef MATERIALS_H
#define MATERIALS_H

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"
#include "../HemisphereSampling.h"

class SampleMaterial:
	public Material
{
public:
	SampleMaterial(const glm::vec3 &_reflRate, float _roughness):
		reflRate(_reflRate), roughness(_roughness), Material(BXDF::REFLECTION) {}

	glm::vec3 outRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		glm::vec3 H = glm::normalize(si.Wi + si.Wo);
		glm::vec3 F = Math::fresnelSchlickRoughness(std::max(glm::dot(H, si.Wo), 0.0f), reflRate, roughness);

		return radiance * F * std::max(0.0f, glm::dot(si.N, si.Wi));
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		return HemisphereSampling::random(N, pdf);
	}

private:
	glm::vec3 reflRate;
	float roughness;
};

class Lambertian:
	public Material
{
public:
	Lambertian(const glm::vec3 &albedo): albedo(albedo), Material(BXDF::REFLECTION) {}

	glm::vec3 outRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		return radiance * albedo * glm::max(glm::dot(si.Wi, si.N), 0.0f) / glm::pi<float>();
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		return HemisphereSampling::cosineWeighted(N, pdf);
	}

private:
	glm::vec3 albedo;
};

class MaterialPBR:
	public Material
{
public:
	MaterialPBR(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness), Material(BXDF::REFLECTION)
	{}

	glm::vec3 outRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		glm::vec3 L = si.Wi;
		glm::vec3 V = si.Wo;
		glm::vec3 N = si.N;
		glm::vec3 H = glm::normalize(V + L);

		float NdotL = glm::max(glm::dot(N, L), 0.0f);
		float NdotV = glm::max(glm::dot(N, V), 0.0f);

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = Math::fresnelSchlickRoughness(glm::max(glm::dot(H, V), 0.0f), F0, roughness);
		float	  D = Math::distributionGGX(N, H, roughness);
		float	  G = Math::geometrySmith(N, V, L, roughness);

		glm::vec3 kS = F;
		glm::vec3 kD = glm::vec3(1.0f) - kS;
		kD *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denominator = 4.0f * NdotV * NdotL + 1e-12f;
		glm::vec3 specular = FDG / denominator;

		auto out = (kD * albedo / glm::pi<float>() + specular) * radiance * NdotL;
		return out;
	}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		RandomGenerator rg;
		float rd = rg.get(0.0f, 1.0f);
		bool sampleDiffuse = (rd < 0.5f * (1.0f - metallic));

		auto sample = sampleDiffuse ? HemisphereSampling::cosineWeighted(N, pdf) : HemisphereSampling::GGX(N, Wo, roughness, pdf);
		pdf *= sampleDiffuse ? 0.5f * (1.0f - metallic) : 0.5f * (1.0f + metallic);

		return sample;
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;
};

class Dieletric:
	public Material
{
public:
	Dieletric(float etaB, float etaA = 1.0f):
		etaB(etaB), etaA(etaA), Material(BXDF::TRANSMISSION) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		bool entering = glm::dot(N, Wo) > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(Wo, N);
		float portionReflect = Math::fresnelDieletric(glm::abs(glm::dot(N, Wo)), etaI, etaT);
		// 如果Wi是反射过来的，那么可以直接用Wi对称地计算出反射的比例

		glm::vec3 dirRefract;
		bool refract = Math::refract(dirRefract, Wo, N, etaI / etaT);

		float portionRefract = 0.0f;
		if (refract)
		{
			float cosTi = glm::abs(glm::dot(N, dirRefract));
			portionRefract = 1.0f - Math::fresnelDieletric(cosTi, etaT, etaI);
			// Wi也有可能是折射过来的，这时候需要反过来算折射的比例
			RandomGenerator rg;
			float sum = portionRefract + portionReflect;
			bool sampleRefract = rg.get(0.0f, 1.0f) < portionRefract / sum;
			//pdf = 0.5f;
			pdf = sampleRefract ? portionRefract / sum : portionReflect / sum;
			return sampleRefract ? dirRefract : dirReflect;
		}

		pdf = 1.0f;
		return dirReflect;
	}

	glm::vec3 outRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		float cosTi = glm::dot(si.N, si.Wi);
		bool entering = cosTi > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		float reflectRatio = Math::fresnelDieletric(glm::abs(cosTi), etaI, etaT);
		bool reflect = cosTi * glm::dot(si.N, si.Wo) >= 0.0f;

		glm::vec3 res = radiance;
		if (reflect) res *= reflectRatio;
		else res *= (1.0f - reflectRatio);
		return res;
	}

private:
	float etaA, etaB;
	// etaA外面, etaB内面
};

class MixedMaterial:
	public Material
{
public:
	MixedMaterial(std::shared_ptr<Material> ma, std::shared_ptr<Material> mb, float mix):
		ma(ma), mb(mb), mix(mix), Material(ma->bxdf().type() | mb->bxdf().type()) {}

	glm::vec3 getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo, float &pdf)
	{
		RandomGenerator rg;

		auto sampleMaterial = (rg.get(0.0f, 1.0f) < mix) ? ma : mb;
		glm::vec3 sample = sampleMaterial->getSample(hitPoint, N, Wo, pdf);
		return sample;
	}

	glm::vec3 outRadiance(const SurfaceInteraction &si, const glm::vec3 &radiance)
	{
		glm::vec3 radianceA = ma->outRadiance(si, radiance);
		glm::vec3 radianceB = mb->outRadiance(si, radiance);
		return Math::lerp(radianceA, radianceB, mix);
	}

private:
	std::shared_ptr<Material> ma, mb;
	float mix;
};

#endif
