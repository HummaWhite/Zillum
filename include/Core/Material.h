#pragma once

#include <iostream>
#include <optional>
#include <random>
#include <cmath>

#include "../../ext/glmIncluder.h"
#include "BXDF.h"
#include "Microfacet.h"
#include "SurfaceInfo.h"
#include "Ray.h"
#include "Math.h"
#include "Transform.h"
#include "PiecewiseDistrib.h"

enum class TransportMode
{
	Radiance, Importance
};

struct BSDFSample
{
	BSDFSample(const Vec3f &dir, float pdf, BXDF type, const Spectrum &bsdf, float eta = 1.0f):
		dir(dir), pdf(pdf), type(type), bsdf(bsdf), eta(eta) {}

	Vec3f dir;
	float pdf;
	BXDF type;
	float eta;
	Spectrum bsdf;
};

class Material
{
public:
	Material(int bxdfType): mMatBxdf(bxdfType) {}

	virtual Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode = TransportMode::Radiance) = 0;
	virtual float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode = TransportMode::Radiance) = 0;
	virtual std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode = TransportMode::Radiance) = 0;

	const BXDF& bxdf() const { return mMatBxdf; }

protected:
	BXDF mMatBxdf;
};

using MaterialPtr = std::shared_ptr<Material>;

class Lambertian:
	public Material
{
public:
	Lambertian(const Spectrum &albedo) :
		albedo(albedo), Material(BXDF::Diffuse) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	Spectrum albedo;
};

class MetalWorkflow:
	public Material
{
public:
	MetalWorkflow(const Spectrum &albedo, float metallic, float roughness) :
		albedo(albedo), metallic(metallic), roughness(roughness),
		distrib(roughness, true), Material(BXDF::Diffuse | BXDF::GlosRefl) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	bool approxDelta() const { return roughness <= 0.014f; }

private:
	Spectrum albedo;
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

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	GTR1Distrib distrib;
	float weight;
};

class Dielectric:
	public Material
{
public:
	Dielectric(const Spectrum &baseColor, float roughness, float ior):
		baseColor(baseColor), ior(ior), distrib(roughness, false),
		approxDelta(roughness < 0.014f), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	float ior;
	Spectrum baseColor;
	GGXDistrib distrib;
	bool approxDelta;
};

class ThinDielectric:
	public Material
{
public:
	ThinDielectric(const Spectrum &baseColor, float ior):
		baseColor(baseColor), ior(ior), Material(BXDF::SpecRefl | BXDF::SpecTrans) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode) { return 0.0f; }
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float ior;
};

class DisneyDiffuse :
	public Material
{
public:
	DisneyDiffuse(const Spectrum &baseColor, float roughness, float subsurface) :
		baseColor(baseColor), roughness(roughness), subsurface(subsurface),
		Material(BXDF::Diffuse) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float roughness;
	float subsurface;
};

class DisneyMetal :
	public Material
{
public:
	DisneyMetal(const Spectrum &baseColor, float roughness, float anisotropic = 0.0f) :
		baseColor(baseColor), distrib(roughness, true, anisotropic), Material(BXDF::GlosRefl) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	GGXDistrib distrib;
};

class DisneyClearcoat :
	public Material
{
public:
	DisneyClearcoat(float gloss) : alpha(glm::mix(0.1f, 0.001f, gloss)), distrib(alpha),
		Material(BXDF::GlosRefl) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	float alpha;
	GTR1Distrib distrib;
};

class DisneySheen :
	public Material
{
public:
	DisneySheen(const Spectrum &baseColor, float tint) :
		baseColor(baseColor), tint(tint), Material(BXDF::GlosRefl) {}

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float tint;
};

class DisneyBSDF :
	public Material
{
public:
	DisneyBSDF(
		const Spectrum &baseColor = Vec3f(1.0f),
		float subsurface = 0.0f,
		float metallic = 0.0f,
		float roughness = 0.5f,
		float specular = 0.5f,
		float specularTint = 0.0f,
		float sheen = 0.0f,
		float sheenTint = 0.0f,
		float clearcoat = 0.0f,
		float clearcoatGloss = 0.0f,
		float transmission = 0.0f,
		float transmissionRoughness = 0.013f,
		float ior = 1.5f
	);

	Spectrum bsdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	float pdf(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, TransportMode mode);
	std::optional<BSDFSample> sample(const Vec3f &n, const Vec3f &wo, const Vec3f &u, TransportMode mode);

private:
	DisneyDiffuse diffuse;
	DisneyMetal metal;
	DisneyClearcoat clearcoat;
	DisneySheen sheen;
	Dielectric dielectric;
	Piecewise1D piecewiseSampler;
	float weights[5];
	Material *components[5];
};

bool refract(Vec3f &Wt, const Vec3f &wi, const Vec3f &n, float eta);
float fresnelDielectric(float cosTi, float eta);