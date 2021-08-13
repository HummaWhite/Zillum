#pragma once

#include "../Math/Math.h"

namespace Microfacet
{
    float schlickG(float cosTheta, float alpha);
    float smithG(float cosThetaO, float cosThetaI, float alpha);
    float smithG(const glm::vec3 &N, const glm::vec3 &Wo, const glm::vec3 &Wi, float alpha);

    float ggx(float cosTheta, float alpha);
    float ggx(float cosTheta, float sinPhi, const glm::vec2 &alph);
    float gtr1(float cosTheta, float alpha);
    
    glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0);
    glm::vec3 schlickF(float cosTheta, const glm::vec3 &F0, float roughness);
}