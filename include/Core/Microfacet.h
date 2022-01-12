#pragma once

#include "Transform.h"
#include "Math.h"

class NormalDistrib
{
public:
	virtual float d(const Vec3f &N, const Vec3f &M) = 0;
	virtual float pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo) = 0;
	virtual Vec3f sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u) = 0;
};

class GGXDistrib:
	public NormalDistrib
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
	public NormalDistrib
{
public:
	GTR1Distrib(float roughness) : alpha(roughness * roughness) {}

	float d(const Vec3f &N, const Vec3f &M);
	float pdf(const Vec3f &N, const Vec3f &M, const Vec3f &Wo);
	Vec3f sampleWm(const Vec3f &N, const Vec3f &Wo, const Vec2f &u);

private:
	float alpha;
};

float schlickG(float cosTheta, float alpha);
float smithG(float cosThetaO, float cosThetaI, float alpha);
float smithG(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, float alpha);

float ggx(float cosTheta, float alpha);
float ggx(float cosTheta, float sinPhi, const glm::vec2 &alph);
float gtr1(float cosTheta, float alpha);

float schlickW(float cosTheta);
float schlickF(float cosTheta);
Vec3f schlickF(float cosTheta, const Vec3f &F0);
Vec3f schlickF(float cosTheta, const Vec3f &F0, float roughness);