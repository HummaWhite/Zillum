#pragma once

#include "../../Utils/NamespaceDecl.h"
#include "../../Math/Math.h"

NAMESPACE_BEGIN(Microfacet)

float schlickG(float cosTheta, float alpha);
float smithG(float cosThetaO, float cosThetaI, float alpha);
float smithG(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, float alpha);

float ggx(float cosTheta, float alpha);
float ggx(float cosTheta, float sinPhi, const glm::vec2 &alph);
float gtr1(float cosTheta, float alpha);

Vec3f schlickF(float cosTheta, const Vec3f &F0);
Vec3f schlickF(float cosTheta, const Vec3f &F0, float roughness);

NAMESPACE_END(Microfacet)