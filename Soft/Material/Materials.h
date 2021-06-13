#pragma once

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
		return albedo * Math::PiInv;
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return glm::dot(Wi, N) * Math::PiInv;
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		auto [Wi, pdf] = Math::sampleHemisphereCosine(N);
		return Sample(Wi, pdf, BXDF::Diffuse);
	}

private:
	glm::vec3 albedo;
};

class MetalWorkflow:
	public Material
{
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

		glm::vec3 F = Microfacet::schlickF(Math::satDot(H, Wo), F0, roughness);
		float	  D = ggxDistrib.d(N, H);
		float	  G = ggxDistrib.g(N, Wo, Wi);

		glm::vec3 ks = F;
		glm::vec3 kd = glm::vec3(1.0f) - ks;
		kd *= 1.0f - metallic;

		glm::vec3 FDG = F * D * G;
		float denom = 4.0f * NoV * NoL;
		if (denom < 1e-7f) return glm::vec3(0.0f);

		glm::vec3 glossy = FDG / denom;

		return kd * albedo * Math::PiInv + glossy;
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		float NoWi = glm::dot(N, Wi);
		glm::vec3 H = glm::normalize(Wo + Wi);

		float pdfDiff = NoWi * Math::PiInv;
		float pdfSpec = ggxDistrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
		return Math::lerp(pdfDiff, pdfSpec, 1.0f / (2.0f - metallic));
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		float spec = 1.0f / (2.0f - metallic);
		bool sampleDiff = uniformFloat() > spec;

		glm::vec3 Wi;
		if (sampleDiff) Wi = Math::sampleHemisphereCosine(N).first;
		else
		{
			auto H = ggxDistrib.sampleWm(N, Wo);
			Wi = glm::reflect(-Wo, H);
		}

		float NoWi = glm::dot(N, Wi);
		if (NoWi < 0.0f) return Sample();

		return Sample(Wi, pdf(Wo, Wi, N), sampleDiff ? BXDF::Diffuse : BXDF::GlosRefl);
	}

private:
	glm::vec3 albedo;
	float metallic;
	float roughness;
	GGXDistrib ggxDistrib;
};

class Clearcoat:
	public Material
{
public:
	Clearcoat(float roughness, float weight):
		distrib(roughness), weight(weight), Material(BXDF::GlosRefl) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		auto [Wo, Wi, N, uv] = si;
		auto H = glm::normalize(Wo + Wi);

		float NoWo = Math::satDot(N, Wo);
		float NoWi = Math::satDot(N, Wi);

		float D = distrib.d(N, H);
		auto F = Microfacet::schlickF(Math::absDot(H, Wo), glm::vec3(0.04f));
		float G = Microfacet::smithG(N, Wo, Wi, 0.25f);

		float denom = 4.0f * NoWo * NoWi;
		if (denom < 1e-7f) return glm::vec3(0.0f);

		return F * D * G * weight / denom;
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		auto H = glm::normalize(Wo + Wi);
		return distrib.pdf(N, H, Wo) / (4.0f * glm::dot(H, Wo));
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		auto H = distrib.sampleWm(N, Wo);
		auto Wi = glm::reflect(-Wo, H);

		if (glm::dot(N, Wi) < 0.0f) return Sample();

		return Sample(Wi, pdf(Wo, Wi, N), BXDF::GlosRefl);
	}

private:
	GTR1Distrib distrib;
	float weight;
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

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		if (approximateDelta) return glm::vec3(0.0f);
		auto [Wo, Wi, N, uv] = si;
		auto H = glm::normalize(Wo + Wi);

		float HoWo = Math::absDot(H, Wo);
		float HoWi = Math::absDot(H, Wi);

		if (Math::sameHemisphere(N, Wo, Wi))
		{
			float refl = fresnelDielectric(Math::absDot(H, Wi), ior);
			return (HoWo * HoWi < 1e-7f) ? glm::vec3(0.0f) : tint * ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) / (4.0f * HoWo * HoWi) * refl;
		}
		else
		{
			float eta = glm::dot(N, Wi) > 0 ? ior : 1.0f / ior;
			float sqrtDenom = glm::dot(H, Wo) + eta * glm::dot(H, Wi);
			float denom = sqrtDenom * sqrtDenom;
			float dHdWi = HoWi / denom;

			denom *= Math::absDot(N, Wi) * Math::absDot(N, Wo);
			float refl = fresnelDielectric(glm::dot(H, Wi), eta);

			return (denom < 1e-7f) ? glm::vec3(0.0f) : tint * glm::abs(ggxDistrib.d(N, H) * ggxDistrib.g(N, Wo, Wi) * HoWo * HoWi) / denom * (1.0f - refl);
		}
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		if (approximateDelta) return 0;
		
		if (Math::sameHemisphere(N, Wo, Wi))
		{
			auto H = glm::normalize(Wo + Wi);
			if (glm::dot(Wo, H) < 0) return 0;

			float refl = fresnelDielectric(Math::absDot(H, Wi), ior);
			return ggxDistrib.pdf(N, H, Wo) / (4.0f * Math::absDot(H, Wo)) * refl;
		}
		else
		{
			float eta = glm::dot(N, Wo) > 0 ? ior : 1.0f / ior;
			auto H = glm::normalize(Wo + Wi * eta);
			if (Math::sameHemisphere(H, Wo, Wi)) return 0;

			float trans = 1.0f - fresnelDielectric(Math::absDot(H, Wo), eta);
			float dHdWi = Math::absDot(H, Wi) / Math::square(glm::dot(H, Wo) + eta * glm::dot(H, Wi));
			return ggxDistrib.pdf(N, H, Wo) * dHdWi * trans;
		}
	}

	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo) override
	{
		if (approximateDelta)
		{
			float refl = fresnelDielectric(glm::dot(N, Wo), ior), trans = 1 - refl;

			if (uniformFloat() < refl)
			{
				glm::vec3 Wi = -glm::reflect(Wo, N);
				return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
			}
			else
			{
				glm::vec3 Wi;
				bool refr = refract(Wi, Wo, N, ior);
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

			if (uniformFloat() < refl)
			{
				auto Wi = -glm::reflect(Wo, H);
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
				bool refr = refract(Wi, Wo, H, ior);
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

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample();
	}

	glm::vec4 getSampleForward(const glm::vec3 &N, const glm::vec3 &Wi)
	{
		float eta = (glm::dot(N, Wi)) ? ior : 1.0f / ior;

		glm::vec3 dirReflect = -glm::reflect(Wi, N), dirRefract;
		float portionReflect = fresnelDielectric(Math::absDot(N, Wi), eta);
		refract(dirRefract, Wi, N, eta);

		return glm::vec4(uniformFloat() < portionReflect ? dirReflect : dirRefract, 0.5f);
	}

private:
	float ior;
	glm::vec3 tint;
	GGXDistrib ggxDistrib;
	bool approximateDelta;
};

class ThinDielectric:
	public Material
{
public:
	ThinDielectric(const glm::vec3 &tint, float ior):
		tint(tint), ior(ior), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
	{
		return glm::vec3(0.0f);
	}

	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo) override
	{
		float refl = fresnelDielectric(glm::dot(N, Wo), ior);
		float trans = 1.0f - refl;
		if (refl < 1.0f)
		{
			refl += trans * trans * refl / (1.0f - refl * refl);
			trans = 1.0f - refl;
		}

		float r = uniformFloat();
		if (r < refl)
		{
			auto Wi = glm::reflect(-Wo, N);
			return SampleWithBsdf(Sample(Wi, 1.0f, BXDF::SpecRefl), tint);
		}
		else
		{
			return SampleWithBsdf(Sample(-Wo, 1.0f, BXDF::SpecTrans), tint);
		}
	}

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		return Sample();
	}

	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
	{
		return 0.0f;
	}

private:
	glm::vec3 tint;
	float ior;
};

class MixedMaterial:
	public Material
{
public:
	MixedMaterial(std::shared_ptr<Material> ma, std::shared_ptr<Material> mb, float mix):
		ma(ma), mb(mb), mix(mix), Material(ma->bxdf().type() | mb->bxdf().type()) {}

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

	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
	{
		auto sampleMaterial = (uniformFloat() < mix) ? ma : mb;
		auto sample = sampleMaterial->getSample(N, Wo);
		glm::vec3 Wi = sample.dir;

		return Sample(glm::vec4(Wi, this->pdf(Wo, Wi, N)), sample.type.type());
	}

private:
	std::shared_ptr<Material> ma, mb;
	float mix;
};
