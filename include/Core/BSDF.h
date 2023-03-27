#pragma once

#include <iostream>
#include <optional>
#include <random>
#include <cmath>

#include "glmIncluder.h"
#include "Microfacet.h"
#include "SurfaceInfo.h"
#include "Ray.h"
#include "Math.h"
#include "Transform.h"
#include "PiecewiseDistrib.h"
#include "Texture.h"
#include "Utils/EnumBitField.h"

class TransportMode {
public:
	enum : uint8_t {
		Radiance = 0, Importance = 1
	};

	TransportMode() = default;

	TransportMode(uint8_t mode) : mode(mode) {}

	bool operator == (uint8_t rhs) const {
		return mode == rhs;
	}

	TransportMode operator ~ () const {
		return { !mode };
	}

	bool isAdjoint() const {
		return mode == Importance;
	}

private:
	uint8_t mode;
};

class BSDFType {
public:
	enum Type {
		None = 0,
		Reflection =    0b00000001,
		Transmission =  0b00000010,
		DirectionMask = 0b00000111,
		Delta =         0b00010000,
		Glossy =        0b00100000,
		Diffuse =       0b01000000,
		TypeMask =      0b11111000,
		AllMask =       0b11111111,
	};

public:
	BSDFType(int type) : type(type) {}

	bool hasType(BSDFType type) const {
		return this->type & type.type;
	}

	bool isReflection() const {
		return isReflection(type);
	}

	bool isTransmission() const {
		return isTransmission(type);
	}

	bool isDelta() const {
		return isDelta(type);
	}

	static bool isReflection(BSDFType type) {
		return isExlusive(type, Reflection, DirectionMask);
	}

	static bool isTransmission(BSDFType type) {
		return isExlusive(type, Transmission, DirectionMask);
	}

	static bool isDelta(BSDFType type) {
		return isExlusive(type, Delta, TypeMask);
	}

private:
	static bool isExlusive(BSDFType type, int target, int mask) {
		return !((type.type - (type.type & target)) & mask);
	}

public:
	int type;
};

// Vec3f dir, Spectrum bsdf, float pdf, BSDFType type, float eta
struct BSDFSample {
	BSDFSample() : type(BSDFType::AllMask) {}

	BSDFSample(const Vec3f &dir, const Spectrum& bsdf, float pdf, BSDFType type, float eta = 1.0f) :
		dir(dir), pdf(pdf), type(type), bsdf(bsdf), eta(eta) {}

	Vec3f dir;
	Spectrum bsdf;
	float pdf;
	BSDFType type;
	float eta;
};

class BSDF {
public:
	BSDF(BSDFType type): mType(type) {}

	virtual Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode = TransportMode::Radiance) = 0;
	virtual float pdf(const SurfaceIntr &intr, TransportMode mode = TransportMode::Radiance) = 0;
	virtual std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode = TransportMode::Radiance) = 0;

	virtual BSDFType type() const { return mType; }

protected:
	BSDFType mType;
};

using BSDFPtr = std::shared_ptr<BSDF>;

class FakeBSDF : public BSDF {
public:
	FakeBSDF() : BSDF(BSDFType::Delta | BSDFType::Transmission) {}

	Spectrum bsdf(const SurfaceIntr& intr, TransportMode mode) {
		return Spectrum(0.f);
	}
	float pdf(const SurfaceIntr& intr, TransportMode mode) {
		return 0.f;
	}
	std::optional<BSDFSample> sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode) {
		return BSDFSample(-intr.wo, Spectrum(1.f), 1.f, BSDFType::Delta | BSDFType::Transmission);
	}
};

class Lambertian: public BSDF {
public:
	Lambertian(const ColorMap<Vec3f> &albedo) :
		albedo(albedo), BSDF(BSDFType::Diffuse) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	ColorMap<Vec3f> albedo;
};

class Mirror: public BSDF {
public:
	Mirror(const ColorMap<Vec3f> &baseColor) :
		baseColor(baseColor), BSDF(BSDFType::Delta | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	ColorMap<Vec3f> baseColor;
};

class MetalWorkflow: public BSDF {
public:
	MetalWorkflow(const ColorMap<Vec3f> &baseColor, float metallic, float roughness) :
		baseColor(baseColor), metallic(metallic), roughness(roughness),
		distrib(roughness, true), BSDF(BSDFType::Diffuse | BSDFType::Glossy | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	bool approxDelta() const { return roughness <= 0.014f; }

private:
	ColorMap<Vec3f> baseColor;
	float metallic;
	float roughness;
	GTR2Distrib distrib;
};

class Clearcoat: public BSDF {
public:
	Clearcoat(float roughness, float weight):
		distrib(roughness), weight(weight), BSDF(BSDFType::Glossy | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	GTR1Distrib distrib;
	float weight;
};

class Dielectric: public BSDF {
public:
	Dielectric(const Spectrum &baseColor, float roughness, float ior):
		baseColor(baseColor), ior(ior), distrib(roughness, false),
		approxDelta(roughness < 0.014f),
		BSDF((roughness < 0.014f ? BSDFType::Delta : BSDFType::Glossy) | BSDFType::Reflection | BSDFType::Transmission) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	float ior;
	Spectrum baseColor;
	GTR2Distrib distrib;
	bool approxDelta;
};

class ThinDielectric: public BSDF {
public:
	ThinDielectric(const Spectrum &baseColor, float ior):
		baseColor(baseColor), ior(ior), BSDF(BSDFType::Delta | BSDFType::Reflection | BSDFType::Transmission) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode) { return 0.0f; }
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float ior;
};

class DisneyDiffuse : public BSDF {
public:
	DisneyDiffuse(const Spectrum &baseColor, float roughness, float subsurface) :
		baseColor(baseColor), roughness(roughness), subsurface(subsurface),
		BSDF(BSDFType::Diffuse) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float roughness;
	float subsurface;
};

class DisneyMetal : public BSDF {
public:
	DisneyMetal(const Spectrum &baseColor, float roughness, float anisotropic = 0.0f) :
		baseColor(baseColor), distrib(roughness, true, anisotropic), BSDF(BSDFType::Glossy | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	GTR2Distrib distrib;
};

class DisneyClearcoat : public BSDF {
public:
	DisneyClearcoat(float gloss) : alpha(glm::mix(0.1f, 0.001f, gloss)), distrib(alpha),
		BSDF(BSDFType::Glossy | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	float alpha;
	GTR1Distrib distrib;
};

class DisneySheen : public BSDF {
public:
	DisneySheen(const Spectrum &baseColor, float tint) :
		baseColor(baseColor), tint(tint), BSDF(BSDFType::Glossy | BSDFType::Reflection) {}

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	Spectrum baseColor;
	float tint;
};

class DisneyBSDF : public BSDF {
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

	Spectrum bsdf(const SurfaceIntr &intr, TransportMode mode);
	float pdf(const SurfaceIntr &intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr &intr, const Vec3f &u, TransportMode mode);

private:
	DisneyDiffuse diffuse;
	DisneyMetal metal;
	DisneyClearcoat clearcoat;
	DisneySheen sheen;
	Dielectric dielectric;
	Piecewise1D piecewiseSampler;
	float weights[5];
	BSDF *components[5];
};

class LayeredBSDF : public BSDF {
public:
	LayeredBSDF() : BSDF(BSDFType::None) {}

	Spectrum bsdf(const SurfaceIntr& intr, TransportMode mode);
	float pdf(const SurfaceIntr& intr, TransportMode mode);
	std::optional<BSDFSample> sample(const SurfaceIntr& intr, const Vec3f& u, TransportMode mode);

	void addBSDF(BSDF* bsdf, Texture3fPtr normalMap);

public:
	std::vector<BSDF*> interfaces;
	std::vector<Texture3fPtr> normalMaps;
	int maxDepth = 7;
	int pdfEvalTimes = 4;

private:
};

bool refract(Vec3f &wt, const Vec3f &wi, const Vec3f &n, float eta);
float fresnelDielectric(float cosTi, float eta);