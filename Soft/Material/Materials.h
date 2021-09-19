#pragma once

#include <random>
#include <ctime>

#include "Material.h"
#include "../Math/Math.h"

class Lambertian:
	public Material
{
public:
	Lambertian(const Vec3f &albedo) :
		albedo(albedo), Material(BXDF::Diffuse) {}

	Vec3f bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	float pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode);

private:
	Vec3f albedo;
};

class MetalWorkflow:
	public Material
{
public:
	MetalWorkflow(const Vec3f &albedo, float metallic, float roughness) :
		albedo(albedo), metallic(metallic), roughness(roughness),
		distrib(roughness, true), Material(BXDF::Diffuse | BXDF::GlosRefl) {}

	Vec3f bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	float pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode);

private:
	Vec3f albedo;
	float metallic;
	float roughness;
	GGXDistrib distrib;
};

class Clearcoat:
	public Material
{
public:
	Clearcoat(float roughness, float weight):
		distrib(roughness), weight(weight), Material(BXDF::GlosRefl) {}

	Vec3f bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	float pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode);

private:
	GTR1Distrib distrib;
	float weight;
};

class Dielectric:
	public Material
{
public:
	Dielectric(const Vec3f &tint, float roughness, float ior):
		tint(tint), ior(ior), distrib(roughness, false),
		approxDelta(roughness < 0.014f), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	Vec3f bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	float pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode) { return Sample(); }
	SampleWithBsdf sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode) override;

private:
	float ior;
	Vec3f tint;
	GGXDistrib distrib;
	bool approxDelta;
};

class ThinDielectric:
	public Material
{
public:
	ThinDielectric(const Vec3f &tint, float ior):
		tint(tint), ior(ior), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	Vec3f bsdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode);
	float pdf(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, TransportMode mode) { return 0.0f; }
	Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode) { return Sample(); }
	SampleWithBsdf sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode) override;

private:
	Vec3f tint;
	float ior;
};

// class MixedMaterial:
// 	public Material
// {
// public:
// 	MixedMaterial(std::shared_ptr<Material> ma, std::shared_ptr<Material> mb, float mix):
// 		ma(ma), mb(mb), mix(mix), Material(ma->bxdf().type() | mb->bxdf().type()) {}

// 	Vec3f bsdf(const SurfaceInteraction &si, int type)
// 	{
// 		Vec3f radianceA = ma->bsdf(si, type);
// 		Vec3f radianceB = mb->bsdf(si, type);
// 		return Math::lerp(radianceA, radianceB, mix);
// 	}

// 	float pdf(const Vec3f &Wo, const Vec3f &Wi, const Vec3f &N)
// 	{
// 		return Math::lerp(ma->pdf(Wo, Wi, N), mb->pdf(Wo, Wi, N), mix);
// 	}

// 	Sample getSample(const Vec3f &N, const Vec3f &Wo)
// 	{
// 		auto sampleMaterial = (uniformFloat() < mix) ? ma : mb;
// 		auto sample = sampleMaterial->getSample(N, Wo);
// 		Vec3f Wi = sample.dir;

// 		return Sample(glm::vec4(Wi, this->pdf(Wo, Wi, N)), sample.type.type());
// 	}

// private:
// 	std::shared_ptr<Material> ma, mb;
// 	float mix;
// };
