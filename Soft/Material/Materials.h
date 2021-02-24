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

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
	{
		return albedo * glm::max(glm::dot(si.Wi, si.N), 0.0f) / glm::pi<float>();
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
		ggxDistrib(roughness), Material(BXDF::Diffuse | BXDF::Glossy)
	{}

	glm::vec3 bsdf(const SurfaceInteraction &si, uint8_t param)
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

		return (kd * albedo * Math::PiInv + glossy) * NoL;
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		RandomGenerator rg;
		float spec = 1.0f / (2.0f - metallic);
		bool sampleDiff = rg.get() > spec;

		auto Wi = sampleDiff ? Transform::normalToWorld(N, Math::randHemisphere()) : ggxDistrib.sampleVndf(N, Wo);
		if (!sampleDiff) Wi = glm::reflect(-Wo, Wi);

		float NoWi = glm::dot(N, Wi);
		if (NoWi < 0.0f) return Sample();

		return Sample(Wi, pdf(Wo, Wi, N), sampleDiff ? BXDF::Diffuse : BXDF::Glossy);
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		float NoWi = glm::dot(N, Wi);
		glm::vec3 H = glm::normalize(Wo + Wi);

		float pdfDiff = NoWi * Math::PiInv;
		float pdfSpec = ggxDistrib.pdfVndf(N, H, Wo);
		return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;
	GGXDistrib ggxDistrib;
};

class Dieletric:
	public Material
{
public:
	Dieletric(const glm::vec3 &tint, float roughness, float etaB, float etaA = 1.0f):
		tint(tint), etaB(etaB), etaA(etaA),
		ggxDistrib(roughness * roughness), Material(BXDF::Specular | BXDF::SpecTrans)
	{
		approximateDelta = roughness < 0.014f;
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample();
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
		return 0.0f;
	}

	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo) override
	{
		RandomGenerator rg;
		if (approximateDelta)
		{
			float refl = fresnelDieletric(glm::dot(N, Wo), etaA, etaB), trans = 1 - refl;
			if (rg.get() < refl)
			{
				glm::vec3 Wi = -glm::reflect(Wo, N);
				glm::vec3 r(tint * refl /* Math::absDot(N, Wi)*/);
				return SampleWithBsdf(Sample(Wi, refl, BXDF::Specular), r);
			}
			else
			{
				bool entering = glm::dot(N, Wo) > 0.0f;
				float etaI = entering ? etaA : etaB;
				float etaT = entering ? etaB : etaA;

				glm::vec3 Wi;
				bool refr = refract(Wi, Wo, N, etaI / etaT);
				if (!refr) return SampleWithBsdf(Sample(), glm::vec3(0.0f));

				glm::vec3 r(tint * trans);
				return SampleWithBsdf(Sample(Wi, trans, BXDF::SpecTrans), r);
			}
		}
		else
		{
			glm::vec3 H = ggxDistrib.sampleM(N);
			if (glm::dot(H, Wo) < 0.0f) return SampleWithBsdf(Sample(), glm::vec3(0.0f));
			float refl = fresnelDieletric(glm::dot(N, H), etaA, etaB), trans = 1.0f - refl;

			if (rg.get() < refl)
			{
				auto Wi = glm::reflect(-Wo, H);
				float p = ggxDistrib.pdf(N, H, Wo) * refl / (4.0f * glm::dot(H, Wo));
				float HoWo = Math::absDot(H, Wo);
				float HoWi = Math::absDot(H, Wi);
				glm::vec3 r =
					tint * ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) * refl /
					(4.0f * HoWo * HoWi);

				return SampleWithBsdf(Sample(Wi, p, BXDF::Glossy), r);
			}
			else
			{
				bool entering = glm::dot(N, Wo) > 0.0f;
				float etaI = entering ? etaA : etaB;
				float etaT = entering ? etaB : etaA;

				glm::vec3 Wi;
				bool refr = refract(Wi, Wo, H, etaI / etaT);
				if (!refr) return SampleWithBsdf(Sample(), glm::vec3(0.0f));

				float HoWo = Math::absDot(H, Wo);
				float HoWi = Math::absDot(H, Wi);

				float sqrtDenom = glm::sqrt(glm::dot(H, Wo) + etaI / etaT * glm::dot(H, Wi));
				glm::vec3 r =
					tint * trans * 
					glm::abs(ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) * HoWo * HoWi) /
					(Math::absDot(N, Wi) * Math::absDot(N, Wo) * sqrtDenom);

				float dHdWi = HoWi / sqrtDenom;
				float p = ggxDistrib.pdf(N, H, Wo) * dHdWi * trans;
				return SampleWithBsdf(Sample(Wi, p, BXDF::Glossy), r);
			}
		}
	}

private:
	inline static bool refract(glm::vec3& Wt, const glm::vec3& Wi, const glm::vec3 &N, float eta)
	{
		// 与PBRT不同，这个的结果只与光路上折射率的比值eta有关，与N的取向无关
		float cosTi = glm::dot(N, Wi);
		float sin2Ti = 1.0f - cosTi * cosTi;
		float sin2Tt = eta * eta * sin2Ti;

		if (sin2Tt >= 1.0f) return false;

		float dirN = cosTi < 0 ? -1.0f : 1.0f;
		float cosTt = glm::sqrt(1.0f - sin2Tt) * dirN;
		Wt = glm::normalize(-Wi * eta + N * (eta * cosTi - cosTt));
		return true;
	}

	inline static float fresnelDieletric(float cosTi, float etaI, float etaT)
	{
		cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
		float sinTi = glm::sqrt(1 - cosTi * cosTi);
		float sinTt = etaI / etaT * sinTi;
		if (sinTt >= 1.0f) return 1.0f;
		
		float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

		float rPa = (etaT * cosTi - etaI * cosTt) / (etaT * cosTi + etaI * cosTt);
		float rPe = (etaI * cosTi - etaT * cosTt) / (etaI * cosTi + etaT * cosTt);

		float res = (rPa * rPa + rPe * rPe) * 0.5f;
		return res;
	}

private:
	float etaA, etaB;
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
