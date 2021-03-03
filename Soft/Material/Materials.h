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
	Lambertian(const glm::vec3 &albedo): albedo(albedo), Material(BXDF::Diffuse) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		return albedo * glm::pi<float>();
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		auto v = Math::randHemisphere();
		return Sample(Transform::normalToWorld(N, v), v.z * Math::PiInv, BXDF::Diffuse);
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
	MetalWorkflow(const glm::vec3 &albedo, float metallic, float roughness):
		albedo(albedo), metallic(metallic), roughness(roughness),
		ggxDistrib(roughness, true), Material(BXDF::Diffuse | BXDF::GlosRefl)
	{}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		glm::vec3 Wi = si.Wi;
		glm::vec3 Wo = si.Wo;
		glm::vec3 N = si.N;
		glm::vec3 H = glm::normalize(Wi + Wo);
		float alpha = roughness * roughness;

		if (dot(N, Wi) < 1e-10f) return glm::vec3(0.0f);
		if (dot(N, Wo) < 1e-10f) return glm::vec3(0.0f);

		float NoL = Math::satDot(N, Wi);
		float NoV = Math::satDot(N, Wo);

		glm::vec3 F0 = glm::mix(glm::vec3(0.04f), albedo, metallic);

		glm::vec3 F = schlickF(Math::satDot(H, Wo), F0, roughness);
		float	  D = ggxDistrib.d(N, H);
		float	  G = ggxDistrib.g(N, Wo, Wi);

		glm::vec3 ks = F;
		glm::vec3 kd = glm::vec3(1.0f) - ks;
		kd *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denom = 4.0f * NoV * NoL;
		if (denom < 1e-7f) return glm::vec3(0.0f);

		glm::vec3 glossy = FDG / denom;

		return (kd * albedo * Math::PiInv + glossy);
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;
		float spec = 1.0f / (2.0f - metallic);
		bool sampleDiff = rg.get() > spec;

		glm::vec3 Wi;
		if (sampleDiff) Wi = Transform::normalToWorld(N, Math::randHemisphere());
		else
		{
			auto H = ggxDistrib.sampleWm(N, Wo);
			Wi = glm::reflect(-Wo, H);
		}

		float NoWi = glm::dot(N, Wi);
		if (NoWi < 0.0f) return Sample();

		return Sample(Wi, pdf(Wo, Wi, N), sampleDiff ? BXDF::Diffuse : BXDF::GlosRefl);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		float NoWi = glm::dot(N, Wi);
		glm::vec3 H = glm::normalize(Wo + Wi);

		float pdfDiff = NoWi * Math::PiInv;
		float pdfSpec = ggxDistrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
		return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;
	GGXDistrib ggxDistrib;
};

class Dielectric:
	public Material
{
public:
	Dielectric(const glm::vec3 &tint, float roughness, float ior):
		tint(tint), ior(ior), ggxDistrib(roughness, false), Material(BXDF::SpecRefl | BXDF::SpecTrans)
	{
		approximateDelta = roughness < 0.014f;
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample();
	}

	glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		float eta = (glm::dot(N, Wi)) ? ior : 1.0f / ior;

		glm::vec3 dirReflect = -glm::reflect(Wi, N), dirRefract;
		float portionReflect = fresnelDielectric(glm::abs(glm::dot(N, Wi)), eta);
		refract(dirRefract, Wi, N, eta);

		RandomGenerator rg;
		return glm::vec4(rg.get(0.0f, 1.0f) < portionReflect ? dirReflect : dirRefract, 0.5f);
	}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		float cosTi = glm::dot(si.N, si.Wi);
		float eta = (cosTi > 0.0f) ? ior : 1.0f / ior;

		glm::vec3 dirReflect = -glm::reflect(si.Wi, si.N), dirRefract;
		float reflectRatio = fresnelDielectric(glm::abs(cosTi), eta);

		bool reflect = cosTi * glm::dot(si.N, si.Wo) >= 0.0f;
		if (reflect)
		{
			if (glm::length(dirReflect - si.Wo) > 1e-6f) return glm::vec3(0.0f);
			return glm::vec3(reflectRatio);
		}
		
		refract(dirRefract, si.Wi, si.N, eta);
		if (glm::length(dirRefract - si.Wo) > 1e-6f) return glm::vec3(0.0f);
		return glm::vec3(1.0f - reflectRatio);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return 0.0f;
	}

	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo) override
	{
		RandomGenerator rg;
		if (approximateDelta)
		{
			float refl = fresnelDielectric(glm::dot(N, Wo), ior), trans = 1 - refl;

			if (rg.get() < refl)
			{
				glm::vec3 Wi = -glm::reflect(Wo, N);
				return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
			}
			else
			{
				float eta = (glm::dot(N, Wo) > 0.0f) ? ior : 1.0f / ior;

				glm::vec3 Wi;
				bool refr = refract(Wi, Wo, N, eta);
				if (!refr) return INVALID_BSDF_SAMPLE;

				return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecTrans), tint);
			}
		}
		else
		{
			glm::vec3 H = ggxDistrib.sampleWm(N, Wo);
			if (glm::dot(N, H) < 0.0f) H = -H;
			float refl = fresnelDielectric(glm::dot(H, Wo), ior);
			float trans = 1.0f - refl;

			if (rg.get() < refl)
			{
				auto Wi = -glm::reflect(Wo, H);
				//if (glm::dot(H, Wo) <= 0.0f) return INVALID_BSDF_SAMPLE;
				if (!Math::sameHemisphere(N, Wo, Wi)) return INVALID_BSDF_SAMPLE;

				float p = ggxDistrib.pdf(N, H, Wo) / (4.0f * Math::absDot(H, Wo));
				float HoWo = Math::absDot(H, Wo);
				float HoWi = Math::absDot(H, Wi);

				glm::vec3 r = (HoWo * HoWi < 1e-7f) ? glm::vec3(0.0f) :
					tint * ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) /
					(4.0f * HoWo * HoWi);

				if (Math::isNan(p)) p = 0.0f;
				return SampleWithBsdf(Sample(Wi, p, BXDF::GlosRefl), r);
			}
			else
			{
				float eta = (glm::dot(H, Wo) > 0.0f) ? ior : 1.0f / ior;

				glm::vec3 Wi;
				bool refr = refract(Wi, Wo, H, eta);
				if (!refr) return INVALID_BSDF_SAMPLE;
				if (Math::sameHemisphere(N, Wo, Wi)) return INVALID_BSDF_SAMPLE;
				if (Math::absDot(N, Wi) < 1e-10f) return INVALID_BSDF_SAMPLE;

				float HoWo = Math::absDot(H, Wo);
				float HoWi = Math::absDot(H, Wi);

				float sqrtDenom = glm::dot(H, Wo) + eta * glm::dot(H, Wi);
				float denom = sqrtDenom * sqrtDenom;
				float dHdWi = HoWi / denom;

				denom *= Math::absDot(N, Wi) * Math::absDot(N, Wo);

				glm::vec3 r = (denom < 1e-7f) ? glm::vec3(0.0f) :
					tint * 
					glm::abs(ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) * HoWo * HoWi) / denom;

				float p = ggxDistrib.pdf(N, H, Wo) * dHdWi;

				if (Math::isNan(p)) p = 0.0f;
				return SampleWithBsdf(Sample(Wi, p, BXDF::GlosTrans), r);
			}
		}
	}

private:
	inline static bool refract(glm::vec3& Wt, const glm::vec3& Wi, const glm::vec3 &N, float eta)
	{
		float cosTi = glm::dot(N, Wi);
		float sin2Ti = glm::max(0.0f, 1.0f - cosTi * cosTi);
		float sin2Tt = sin2Ti / (eta * eta);

		if (sin2Tt >= 1.0f) return false;

		float dirN = cosTi > 0.0f ? 1.0f : -1.0f;
		float cosTt = glm::sqrt(1.0f - sin2Tt) * dirN;
		Wt = glm::normalize(-Wi / eta + N * dirN * (cosTi / eta - cosTt));
		return true;
	}

	inline static float fresnelDielectric(float cosTi, float eta)
	{
		cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
		if (cosTi < 0.0f)
		{
			eta = 1.0f / eta;
			cosTi = -cosTi;
		}

		float sinTi = glm::sqrt(1.0f - cosTi * cosTi);
		float sinTt = sinTi / eta;
		if (sinTt >= 1.0f) return 1.0f;

		float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

		float rPa = (cosTi - eta * cosTt) / (cosTi + eta * cosTt);
		float rPe = (eta * cosTi - cosTt) / (eta * cosTi + cosTt);
		return (rPa * rPa + rPe * rPe) * 0.5f;
	}

private:
	float ior;
	glm::vec3 tint;
	GGXDistrib ggxDistrib;
	bool approximateDelta;
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

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		glm::vec3 radianceA = ma->bsdf(si, type);
		glm::vec3 radianceB = mb->bsdf(si, type);
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
