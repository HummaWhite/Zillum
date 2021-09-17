#pragma once

#include <iostream>
#include <random>
#include <cmath>

#include "../glm/glmIncluder.h"

#include "../Surface/SurfaceInteraction.h"
#include "../Scene/Ray.h"
#include "../BXDF/BXDF.h"
#include "../Math/Transform.h"
#include "../Microfacet/Microfacets.h"

enum class TransportMode
{
	Radiance, Importance
};

struct Sample
{
	Sample(): dir(0.0f), pdf(0.0f), type(0), eta(0.0f) {}

	Sample(const Vec4f &sample, int type, float eta = 1.0f):
		dir(sample), pdf(sample.w), type(type), eta(eta) {}

	Sample(const Vec3f &dir, float pdf, int type, float eta = 1.0f):
		dir(dir), pdf(pdf), type(type), eta(eta) {}

	Vec3f dir;
	float pdf;
	BXDF type;
	float eta;
};

typedef std::pair<Sample, Vec3f> SampleWithBsdf;
const SampleWithBsdf INVALID_BSDF_SAMPLE = SampleWithBsdf(Sample(), Vec3f(0.0f));

class Material
{
public:
	Material(int bxdfType): matBxdf(bxdfType) {}

	virtual Vec3f bsdf(const SurfaceInteraction &si, TransportMode mode = TransportMode::Radiance) = 0;
	virtual Sample getSample(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode = TransportMode::Radiance) = 0;
	virtual float pdf(const Vec3f &Wo, const Vec3f &Wi, const Vec3f &N, TransportMode mode = TransportMode::Radiance) = 0;

	virtual SampleWithBsdf sampleWithBsdf(const Vec3f &N, const Vec3f &Wo, float u1, const Vec2f &u2, TransportMode mode = TransportMode::Radiance)
	{
		Sample sample = getSample(N, Wo, u1, u2);
		SurfaceInteraction si = { Wo, sample.dir, N };
		Vec3f bsdf = this->bsdf(si, mode);
		return SampleWithBsdf(sample, bsdf);
	}

	virtual Vec4f getSampleForward(const Vec3f &N, const Vec3f &Wi)
	{
		Sample sample = getSample(N, Wi, 0.0f, {});
		return Vec4f(sample.dir, sample.pdf);
	}

	const BXDF& bxdf() const { return matBxdf; }

protected:
	inline static bool refract(Vec3f &Wt, const Vec3f &Wi, const Vec3f &N, float eta)
	{
		float cosTi = glm::dot(N, Wi);
		if (cosTi < 0) eta = 1.0f / eta;
		float sin2Ti = glm::max(0.0f, 1.0f - cosTi * cosTi);
		float sin2Tt = sin2Ti / (eta * eta);

		if (sin2Tt >= 1.0f) return false;

		float cosTt = glm::sqrt(1.0f - sin2Tt);
		if (cosTi < 0) cosTt = -cosTt;
		Wt = glm::normalize(-Wi / eta + N * (cosTi / eta - cosTt));
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

protected:
	BXDF matBxdf;
};

using MaterialPtr = std::shared_ptr<Material>;