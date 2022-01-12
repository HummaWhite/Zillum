#include "../../include/Core/Microfacet.h"

float schlickG(float cosTheta, float alpha)
{
    float k = alpha * 0.5f;
    return cosTheta / (cosTheta * (1.0f - k) + k);
}

float smithG(float cosThetaO, float cosThetaI, float alpha)
{
    return schlickG(cosThetaO, alpha) * schlickG(cosThetaI, alpha);
}

float smithG(const Vec3f &N, const Vec3f &Wo, const Vec3f &Wi, float alpha)
{
    return smithG(Math::absDot(N, Wo), Math::absDot(N, Wi), alpha);
}

float ggx(float cosTheta, float alpha)
{
    if (cosTheta < 1e-6f)
        return 0.0f;

    float a2 = alpha * alpha;
    float nom = a2;
    float denom = cosTheta * cosTheta * (a2 - 1.0f) + 1.0f;
    denom = denom * denom * Math::Pi;

    return nom / denom;
}

float ggx(float cosTheta, float sinPhi, const Vec2f &alph)
{
    if (cosTheta < 1e-6f)
        return 0.0f;

    float sinPhi2 = sinPhi * sinPhi;

    float p = (1.0f - sinPhi2) / (alph.x * alph.x) + sinPhi2 / (alph.y * alph.y);
    float k = 1.0f + (p - 1.0f) * (1.0f - cosTheta * cosTheta);
    k = k * k * Math::Pi * alph.x * alph.y;

    return 1.0f / k;
}

float gtr1(float cosTheta, float alpha)
{
    float a2 = alpha * alpha;
    return (a2 - 1.0f) / (2.0f * Math::Pi * glm::log(alpha) * (1.0f + (a2 - 1.0f) * cosTheta * cosTheta));
}

float schlickW(float cosTheta)
{
    return Math::pow5(1.0f - cosTheta);
}

float schilickF(float cosTheta, float eta)
{
    float F0 = Math::square(eta - 1.0f) / Math::square(eta + 1.0f);
    return F0 + (1.0f - F0) * Math::pow5(1.0f - cosTheta);
}

Vec3f schlickF(float cosTheta, const Vec3f &F0)
{
    return F0 + (Vec3f(1.0f) - F0) * Math::pow5(1.0f - cosTheta);
}

Vec3f schlickF(float cosTheta, const Vec3f &F0, float roughness)
{
    return F0 + (glm::max(Vec3f(1.0f - roughness), F0) - F0) * Math::pow5(1.0f - cosTheta);
}