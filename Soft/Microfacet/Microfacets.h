#pragma once

#include "Microfacet.h"
#include "DistribTerm.h"
#include "../Math/Transform.h"

class GGXDistrib:
	public MicrofacetDistrib
{
public:
	GGXDistrib(float roughness, bool sampleVisible, float aniso = 0.0f);

	float d(const Vec3f &N, const Vec3f &M);
	float pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo);
	Vec3f sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u);
	float g(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi);

private:
	bool visible;
	float alpha;
};

class GTR1Distrib:
	public MicrofacetDistrib
{
public:
	GTR1Distrib(float roughness) : alpha(roughness * roughness) {}

	float d(const Vec3f &N, const Vec3f &M);
	float pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo);
	Vec3f sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u);

private:
	float alpha;
};
