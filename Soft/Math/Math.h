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

	inline static glm::vec2 sphereToPlane(const glm::vec3 &uv)
	{
		float pi = glm::pi<float>();

		float theta = glm::atan(uv.y, uv.x);
		if (theta < 0.0f) theta += pi * 2.0f;
		
		float phi = glm::atan(glm::length(glm::vec2(uv)), uv.z);
		if (phi < 0.0f) phi += pi * 2.0f;

		return { theta / (pi * 2.0f), phi / pi };
	}

	inline static glm::vec3 planeToSphere(const glm::vec2 &uv)
	{
		float theta = uv.x * glm::pi<float>() * 2.0f;
		float phi = uv.y * glm::pi<float>();
		return { cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi) };
	}

	inline static glm::mat3 TBNMatrix(const glm::vec3 &N)
	{
		glm::vec3 T = (glm::abs(N.z) > 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 B = glm::normalize(glm::cross(N, T));
		T = glm::normalize(glm::cross(B, N));
		return glm::mat3(T, B, N);
	}

	inline static unsigned int inverseBits(unsigned int bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
    	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return bits;
	}

	inline static float radicalInverseVDC(unsigned int bits)
	{
    	return float(inverseBits(bits)) * 2.3283064365386963e-10;
	}

	inline static glm::vec2 hammersley(unsigned int i, unsigned int n)
	{
		return glm::vec2((float)i / (float)n, radicalInverseVDC(i));
	}

	inline static glm::vec3 fresnelSchlick(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0) - F0) * (float)glm::pow(1.0 - cosTheta, 5.0);
	}

	inline static glm::vec3 fresnelSchlickRoughness(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0 - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	inline static float distributionGGX(const glm::vec3 &N, const glm::vec3 &H, float roughness)
	{
		float a = roughness * roughness;
		float a2 = a * a;
		float NdotH = std::max(glm::dot(N, H), 0.0f);
		float NdotH2 = NdotH * NdotH;

		float nom = a2;
		float denom = NdotH2 * (a2 - 1.0) + 1.000001f;
		denom = denom * denom * glm::pi<float>();

		return nom / denom;
	}

	inline static float geometrySchlickGGX(float NdotV, float roughness, bool IBL)
	{
		float r = roughness + 1.0f;
		float k = IBL ? roughness * roughness / 2.0f : r * r / 8.0f;

		float nom = NdotV;
		float denom = NdotV * (1.0f - k) + k + 1e-6f;

		return nom / denom;
	}

	inline static float geometrySmith(const glm::vec3 &N, const glm::vec3 &V, const glm::vec3 &L, float roughness, bool IBL)
	{
		float NdotV = glm::max(glm::dot(N, V), 0.0f);
		float NdotL = glm::max(glm::dot(N, L), 0.0f);

		float ggx2 = geometrySchlickGGX(NdotV, roughness, IBL);
		float ggx1 = geometrySchlickGGX(NdotL, roughness, IBL);

		return ggx1 * ggx2;
	}

	inline static glm::vec3 importanceSampleGGX(const glm::vec2 &xi, const glm::vec3 &N, float roughness)
	{
		float r4 = glm::pow(roughness, 4.0f);

		float phi = 2.0 * glm::pi<float>() * xi.x;
		float cosTheta = glm::sqrt((1.0f - xi.y) / (1.000001f + (r4 - 1.0f) * xi.y));
		//当roughness较小的时候，r4约等于0，遇到xi.y = 1.0f的情况，原式分母1.0f + (r4 - 1.0f) * xi.y
		//有可能为零，导致除0输出nan。需加上一个较小的值防止分母为零
		float sinTheta = glm::sqrt(1.0 - cosTheta * cosTheta);

		glm::vec3 H(glm::cos(phi) * sinTheta, glm::sin(phi) * sinTheta, cosTheta);
    	return glm::normalize(TBNMatrix(N) * H);
	}
}

#endif
