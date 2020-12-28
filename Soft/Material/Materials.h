#ifndef MATERIALS_H
#define MATERIALS_H

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"

class SampleMaterial:
	public Material
{
public:
	SampleMaterial(const glm::vec3 &_reflRate, float _roughness):
		reflRate(_reflRate), roughness(_roughness), Material(BXDF::REFLECTION) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		glm::vec3 H = glm::normalize(si.Wi + si.Wo);
		glm::vec3 F = Math::fresnelSchlickRoughness(std::max(glm::dot(H, si.Wo), 0.0f), reflRate, roughness);

		return F * std::max(0.0f, glm::dot(si.N, si.Wi));
	}

	Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample(HemisphereSampling::random(N), 0);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return 1.0f;
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

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		return albedo * glm::max(glm::dot(si.Wi, si.N), 0.0f) / glm::pi<float>();
	}

	Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample(HemisphereSampling::cosineWeighted(N), 0);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return glm::dot(Wo, N);
	}

private:
	glm::vec3 albedo;
};

class MaterialPBR:
	public Material
{
public:
	enum { DIFFUSE = 0b01, SPECULAR = 0b10 };

public:
	MaterialPBR(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness), Material(BXDF::REFLECTION)
	{}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
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

		glm::vec3 res(0.0f);
		if (param & DIFFUSE) res += kD * albedo / glm::pi<float>();
		if (param & SPECULAR) res += specular;

		return res * NdotL;
	}

	Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;
		float rd = rg.get(0.0f, 1.0f);
		bool sampleDiffuse = (rd < 0.5f * (1.0f - metallic));

		auto sample = sampleDiffuse ? HemisphereSampling::cosineWeighted(N) : HemisphereSampling::GGX(N, Wo, roughness);
		uint8_t param = sampleDiffuse ? DIFFUSE : SPECULAR;
		return Sample(sample, param);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		glm::vec3 H = glm::normalize(Wi + Wo);
		float NdotH = glm::max(glm::dot(N, H), 0.0f);
        float HdotWo = glm::max(glm::dot(H, Wo), 0.0f);

        float D = Math::distributionGGX(N, H, roughness);
		float res = D * NdotH / (4.0 * HdotWo + 1e-8f);

		if (HdotWo < 1e-6f) res = 0.0f;
		if (res < 1e-10f) res = 0.0f;
		return res;
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
	enum { REFLECT = 0b01, REFRACT = 0b10 };

public:
	Dieletric(float etaB, float etaA = 1.0f):
		etaB(etaB), etaA(etaA), Material(BXDF::TRANSMISSION) {}

	Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		bool entering = glm::dot(N, Wo) > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(Wo, N);
		//float portionReflect = Math::fresnelDieletric(glm::abs(glm::dot(N, Wo)), etaI, etaT);
		// 如果Wi是反射过来的，那么可以直接用Wi对称地计算出反射的比例

		glm::vec3 dirRefract;
		bool refract = Math::refract(dirRefract, Wo, N, etaI / etaT);

		float portionRefract = 0.0f;
		if (refract)
		{
			float cosTi = glm::abs(glm::dot(N, dirRefract));
			//portionRefract = 1.0f - Math::fresnelDieletric(cosTi, etaT, etaI);
			// Wi也有可能是折射过来的，这时候需要反过来算折射的比例
			RandomGenerator rg;
			//float sum = portionRefract + portionReflect;
			bool sampleRefract = rg.get(0.0f, 1.0f) < 0.5f;//portionRefract / sum;
			//pdf = sampleRefract ? portionRefract / sum : portionReflect / sum;
			return Sample(glm::vec4(sampleRefract ? dirRefract : dirReflect, 0.5f), ~0);
		}

		return Sample(glm::vec4(dirReflect, 1.0f), ~0);
	}

	glm::vec4 getSampleForward(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wi)
	{
		bool entering = glm::dot(N, Wi) > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(Wi, N), dirRefract;
		float portionReflect = Math::fresnelDieletric(glm::abs(glm::dot(N, Wi)), etaI, etaT);
		Math::refract(dirRefract, Wi, N, etaI / etaT);

		RandomGenerator rg;
		return glm::vec4(rg.get(0.0f, 1.0f) < portionReflect ? dirReflect : dirRefract, 0.5f);
	}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		float cosTi = glm::dot(si.N, si.Wi);
		bool entering = cosTi > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(si.Wi, si.N), dirRefract;
		float reflectRatio = Math::fresnelDieletric(glm::abs(cosTi), etaI, etaT);

		bool reflect = cosTi * glm::dot(si.N, si.Wo) >= 0.0f;
		if (reflect)
		{
			if (glm::length(dirReflect - si.Wo) > 1e-6f) return glm::vec3(0.0f);
			return glm::vec3(reflectRatio);
		}
		
		Math::refract(dirRefract, si.Wi, si.N, etaI / etaT);
		if (glm::length(dirRefract - si.Wo) > 1e-6f) return glm::vec3(0.0f);
		return glm::vec3(1.0f - reflectRatio);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return 1.0f;
	}

private:
	float etaA, etaB;
	// etaA外面, etaB内面
};

class MixedMaterial:
	public Material
{
public:
	enum { A = 0b01, B = 0b10 };

public:
	MixedMaterial(std::shared_ptr<Material> ma, std::shared_ptr<Material> mb, float mix):
		ma(ma), mb(mb), mix(mix), Material(ma->bxdf().type() | mb->bxdf().type()) {}

	Sample getSample(const glm::vec3 &hitPoint, const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;

		auto sampleMaterial = (rg.get(0.0f, 1.0f) < mix) ? ma : mb;
		return sampleMaterial->getSample(hitPoint, N, Wo);
	}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		glm::vec3 radianceA = ma->bsdf(si, param);
		glm::vec3 radianceB = mb->bsdf(si, param);
		return Math::lerp(radianceA, radianceB, mix);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return 1.0f;
	}

private:
	std::shared_ptr<Material> ma, mb;
	float mix;
};

#endif
