#ifndef MATERIALS_H
#define MATERIALS_H

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"

class Lambertian:
	public Material
{
public:
	Lambertian(const glm::vec3 &albedo): albedo(albedo), Material(BXDF::REFLECTION) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		return albedo * glm::max(glm::dot(si.Wi, si.N), 0.0f) / glm::pi<float>();
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		auto v = Math::randHemisphere();
		return Sample(Transform::normalToWorld(N, v), v.z * Math::PiInv, 0);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return glm::dot(Wo, N) * Math::PiInv;
	}

private:
	glm::vec3 albedo;
};

class MetalWorkflow:
	public Material
{
public:
	enum { DIFFUSE = 0b01, SPECULAR = 0b10 };

public:
	MetalWorkflow(const glm::vec3 &_albedo, float _metallic, float _roughness):
		albedo(_albedo), metallic(_metallic), roughness(_roughness), Material(BXDF::REFLECTION)
	{}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		glm::vec3 Wi = si.Wi;
		glm::vec3 Wo = si.Wo;
		glm::vec3 N = si.N;
		glm::vec3 H = glm::normalize(Wi + Wo);
		float alpha = roughness * roughness;

		if (dot(N, Wi) < 0.0f) return glm::vec3(0.0f);
		if (dot(N, Wo) < 0.0f) return glm::vec3(0.0f);

		float NoL = Math::satDot(N, Wi);
		float NoV = Math::satDot(N, Wo);

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = schlickF(Math::satDot(H, Wo), F0, roughness);
		float	  D = GGX(Math::satDot(N, H), alpha);
		float	  G = smithG(N, Wo, Wi, alpha);

		glm::vec3 ks = F;
		glm::vec3 kd = glm::vec3(1.0f) - ks;
		kd *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denom = 4.0f * NoV * NoL;
		if (denom < 1e-7f) return glm::vec3(0.0f);

		glm::vec3 glossy = FDG / denom;

		return (kd * albedo * Math::PiInv + glossy) * NoL;
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;
		float spec = 1.0f / (2.0f - metallic);
		bool sampleDiff = rg.get() > spec;

		auto Wi = sampleDiff ? Transform::normalToWorld(N, Math::randHemisphere()) : sampleGGXVNDF(N, Wo, roughness * roughness);

		float NoWi = glm::dot(N, Wi);
		if (NoWi < 0.0f) return Sample(glm::vec3(0.0f), 0.0f, 0);

		return Sample(Wi, pdf(Wo, Wi, N), 0);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		float NoWi = glm::dot(N, Wi);
		glm::vec3 H = glm::normalize(Wo + Wi);

		float pdfDiff = NoWi * Math::PiInv;
		float pdfSpec = pdfGGXVNDF(N, H, Wo, Wi, roughness * roughness);
		//float pdfSpec = pdfGGX(glm::dot(N, H), glm::dot(H, Wo), roughness * roughness);
		float mix = 1.0f / (2.0f - metallic);
		return Math::lerp(pdfDiff, pdfSpec, mix);
	}

private:
	float pdfGGX(float cosTheta, float MoWo, float alpha)
	{
		return GGX(cosTheta, alpha) * cosTheta / (4.0f * MoWo);
	}

	float pdfGGXVNDF(const glm::vec3 &N, const glm::vec3 &M, const glm::vec3 &Wo, const glm::vec3 &Wi, float alpha)
	{
		return GGXVNDF(glm::dot(N, M), glm::dot(N, Wo), Math::satDot(N, Wo), alpha) / (4.0f * glm::dot(N, Wo));
	}

	glm::vec3 sampleGGX(const glm::vec3 &N, const glm::vec3 &Wo, float alpha)
	{
		RandomGenerator rg;
		glm::vec2 xi = Transform::toConcentricDisk(glm::vec2(rg.get(), rg.get()));

		glm::vec3 H = glm::vec3(xi.x, xi.y, glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y)));
		H = glm::normalize(H * glm::vec3(alpha, alpha, 1.0f));
		H = glm::normalize(Transform::normalToWorld(N, H));

		return glm::reflect(-Wo, H);
	}

	glm::vec3 sampleGGXVNDF(const glm::vec3 &N, const glm::vec3 &Wo, float alpha)
	{
		glm::mat3 TBN = Math::TBNMatrix(N);
		glm::mat3 TBNinv = glm::inverse(TBN);

		glm::vec3 Vh = glm::normalize((TBNinv * Wo) * glm::vec3(alpha, alpha, 1.0f));

		float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
		glm::vec3 T1 = lensq > 0.0f ? glm::vec3(-Vh.y, Vh.x, 0.0f) / glm::sqrt(lensq) : glm::vec3(1.0f, 0.0f, 0.0f);
		glm::vec3 T2 = glm::cross(Vh, T1);

		glm::vec2 xi = Transform::toConcentricDisk(Math::randBox());
		float s = 0.5f * (1.0f + Vh.z);
		xi.y = (1.0f - s) * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x)) + s * xi.y;

		glm::vec3 H = T1 * xi.x + T2 * xi.y + Vh * glm::sqrt(glm::max(0.0f, 1.0f - xi.x * xi.x - xi.y * xi.y));
		H = glm::normalize(glm::vec3(H.x * alpha, H.y * alpha, glm::max(0.0f, H.z)));
		H = glm::normalize(TBN * H);

		return glm::reflect(-Wo, H);
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

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		bool entering = glm::dot(N, Wo) > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(Wo, N);
		//float portionReflect = Math::fresnelDieletric(glm::abs(glm::dot(N, Wo)), etaI, etaT);
		// 如果Wi是反射过来的，那么可以直接用Wi对称地计算出反射的比例

		glm::vec3 dirRefract;
		bool refr = refract(dirRefract, Wo, N, etaI / etaT);

		float portionRefract = 0.0f;
		if (refr)
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

	glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		bool entering = glm::dot(N, Wi) > 0.0f;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;

		glm::vec3 dirReflect = -glm::reflect(Wi, N), dirRefract;
		float portionReflect = fresnelDieletric(glm::abs(glm::dot(N, Wi)), etaI, etaT);
		refract(dirRefract, Wi, N, etaI / etaT);

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
		float reflectRatio = fresnelDieletric(glm::abs(cosTi), etaI, etaT);

		bool reflect = cosTi * glm::dot(si.N, si.Wo) >= 0.0f;
		if (reflect)
		{
			if (glm::length(dirReflect - si.Wo) > 1e-6f) return glm::vec3(0.0f);
			return glm::vec3(reflectRatio);
		}
		
		refract(dirRefract, si.Wi, si.N, etaI / etaT);
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

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;

		auto sampleMaterial = (rg.get(0.0f, 1.0f) < mix) ? ma : mb;
		auto sample = sampleMaterial->getSample(N, Wo);
		glm::vec3 Wi = sample.dir;

		return Sample(glm::vec4(Wi, this->pdf(Wo, Wi, N)), 0);
	}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		glm::vec3 radianceA = ma->bsdf(si, param);
		glm::vec3 radianceB = mb->bsdf(si, param);
		return Math::lerp(radianceA, radianceB, mix);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return Math::lerp(ma->pdf(Wo, Wi, N), mb->pdf(Wo, Wi, N), mix);
	}

private:
	std::shared_ptr<Material> ma, mb;
	float mix;
};

#endif
