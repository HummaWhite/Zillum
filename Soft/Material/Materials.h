#pragma once

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"

class Lambertian:
	public Material
{
public:
	Lambertian(const glm::vec3 &albedo) :
		albedo(albedo), Material(BXDF::Diffuse) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type);
	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N);
	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2);

private:
	glm::vec3 albedo;
};

class MetalWorkflow:
	public Material
{
public:
	MetalWorkflow(const glm::vec3 &albedo, float metallic, float roughness) :
		albedo(albedo), metallic(metallic), roughness(roughness),
		ggxDistrib(roughness, true), Material(BXDF::Diffuse | BXDF::GlosRefl) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type);
	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N);
	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2);

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

	glm::vec3 bsdf(const SurfaceInteraction &si, int type);
	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N);
	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2);

private:
	GTR1Distrib distrib;
	float weight;
};

class Dielectric:
	public Material
{
public:
	Dielectric(const glm::vec3 &tint, float roughness, float ior):
		tint(tint), ior(ior), ggxDistrib(roughness, false),
		approximateDelta(roughness < 0.014f), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	glm::vec3 bsdf(const SurfaceInteraction &si, int type);
	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N);
	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2) { return Sample(); }
	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2) override;

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

	glm::vec3 bsdf(const SurfaceInteraction &si, int type);
	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N) { return 0.0f; }
	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2) { return Sample(); }
	SampleWithBsdf sampleWithBsdf(const glm::vec3 &N, const glm::vec3 &Wo, float u1, const glm::vec2 &u2) override;

private:
	glm::vec3 tint;
	float ior;
};

// class MixedMaterial:
// 	public Material
// {
// public:
// 	MixedMaterial(std::shared_ptr<Material> ma, std::shared_ptr<Material> mb, float mix):
// 		ma(ma), mb(mb), mix(mix), Material(ma->bxdf().type() | mb->bxdf().type()) {}

// 	glm::vec3 bsdf(const SurfaceInteraction &si, int type)
// 	{
// 		glm::vec3 radianceA = ma->bsdf(si, type);
// 		glm::vec3 radianceB = mb->bsdf(si, type);
// 		return Math::lerp(radianceA, radianceB, mix);
// 	}

// 	float pdf(const glm::vec3 &Wo, const glm::vec3 &Wi, const glm::vec3 &N)
// 	{
// 		return Math::lerp(ma->pdf(Wo, Wi, N), mb->pdf(Wo, Wi, N), mix);
// 	}

// 	Sample getSample(const glm::vec3 &N, const glm::vec3 &Wo)
// 	{
// 		auto sampleMaterial = (uniformFloat() < mix) ? ma : mb;
// 		auto sample = sampleMaterial->getSample(N, Wo);
// 		glm::vec3 Wi = sample.dir;

// 		return Sample(glm::vec4(Wi, this->pdf(Wo, Wi, N)), sample.type.type());
// 	}

// private:
// 	std::shared_ptr<Material> ma, mb;
// 	float mix;
// };
