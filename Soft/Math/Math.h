#ifndef MATH_H
#define MATH_H

#include <iostream>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"
#include "RandomGenerator.h"

namespace Math
{
	template<typename T>
	T lerp(T x, T y, float a)
	{
		return x + (y - x) * a;
	}

	static glm::vec2 sphereToPlane(const glm::vec3 &uv)
	{
		float pi = glm::pi<float>();

		float theta = glm::atan(uv.y, uv.x);
		if (theta < 0.0f) theta += pi * 2.0f;
		
		float phi = glm::atan(glm::length(glm::vec2(uv)), uv.z);
		if (phi < 0.0f) phi += pi * 2.0f;

		return { theta / (pi * 2.0f), phi / pi };
	}

	static glm::vec3 planeToSphere(const glm::vec2 &uv)
	{
		float theta = uv.x * glm::pi<float>() * 2.0f;
		float phi = uv.y * glm::pi<float>();
		return { cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi) };
	}

	static float radicalInverseVDC(unsigned int bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
    	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    	return float(bits) * 2.3283064365386963e-10;
	}

	static glm::vec2 hammersley(unsigned int i, unsigned int n)
	{
		return glm::vec2((float)i / (float)n, radicalInverseVDC(i));
	}

	static glm::vec3 fresnelSchlick(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0) - F0) * (float)glm::pow(1.0 - cosTheta, 5.0);
	}

	static glm::vec3 fresnelSchlickRoughness(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0 - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	static float distributionGGX(const glm::vec3 &N, const glm::vec3 &H, float roughness)
	{
		float a = roughness * roughness;
		float a2 = a * a;
		float NdotH = std::max(glm::dot(N, H), 0.0f);
		float NdotH2 = NdotH * NdotH;

		float nom = a2;
		float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
		denom = denom * denom * glm::pi<float>();

		return nom / denom;
	}

	static float geometrySchlickGGX(float NdotV, float roughness, bool IBL)
	{
		float r = roughness + 1.0f;
		float k = IBL ? roughness * roughness / 2.0f : r * r / 8.0f;

		float nom = NdotV;
		float denom = NdotV * (1.0f - k) + k;

		return nom / denom;
	}

	static float geometrySmith(const glm::vec3 &N, const glm::vec3 &V, const glm::vec3 &L, float roughness, bool IBL)
	{
		float NdotV = std::max(glm::dot(N, V), 0.0f);
		float NdotL = std::max(glm::dot(N, L), 0.0f);

		float ggx2 = geometrySchlickGGX(NdotV, roughness, IBL);
		float ggx1 = geometrySchlickGGX(NdotL, roughness, IBL);

		return ggx1 * ggx2;
	}

	static glm::vec3 min(const glm::vec3 &a, const glm::vec3 &b)
	{
		return
		{
			std::min(a.x, b.x),
			std::min(a.y, b.y),
			std::min(a.z, b.z)
		};
	}

	static glm::vec3 max(const glm::vec3 &a, const glm::vec3 &b)
	{
		return
		{
			std::max(a.x, b.x),
			std::max(a.y, b.y),
			std::max(a.z, b.z)
		};
	}
}

#endif
