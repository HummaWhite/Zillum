#pragma once

#include "Transform.h"
#include "Math.h"

class NormalDistrib {
public:
	virtual float d(const Vec3f &m) const = 0;
	virtual float pdf(const Vec3f &m, const Vec3f &wo) const = 0;
	virtual Vec3f sampleWm(const Vec3f &wo, const Vec2f &u) const = 0;
};

class GTR2Distrib: public NormalDistrib {
public:
	GTR2Distrib(float roughness, bool sampleVisible, float aniso = 0.0f);

	float d(const Vec3f &m) const;
	float pdf(const Vec3f &m, const Vec3f &wo) const;
	Vec3f sampleWm(const Vec3f &wo, const Vec2f &u) const;
	float g(const Vec3f &wo, const Vec3f &wi) const;

private:
	bool visible;
	float alpha;
};

class GTR1Distrib: public NormalDistrib {
public:
	GTR1Distrib(float roughness) : alpha(roughness * roughness) {}

	float d(const Vec3f &m) const;
	float pdf(const Vec3f &m, const Vec3f &wo) const;
	Vec3f sampleWm(const Vec3f &wo, const Vec2f &u) const;

private:
	float alpha;
};

float schlickG(float cosTheta, float alpha);
float smithG(float cosThetaO, float cosThetaI, float alpha);
float smithG(const Vec3f &wo, const Vec3f &wi, float alpha);

float ggx(float cosTheta, float alpha);
float ggx(float cosTheta, float sinPhi, const glm::vec2 &alph);
float gtr1(float cosTheta, float alpha);

float schlickW(float cosTheta);
float schlickF(float cosTheta);
Vec3f SchlickF(float cosTheta, const Vec3f &f0);
Vec3f SchlickF(float cosTheta, const Vec3f &f0, float roughness);