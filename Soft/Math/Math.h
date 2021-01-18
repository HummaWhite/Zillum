#ifndef MATH_H
#define MATH_H

#include <iostream>
#include <iomanip>
#include <cmath>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"
#include "RandomGenerator.h"
#include "ToneMapping.h"

namespace Math
{
	const uint32_t FLOAT_SIG_MASK = 0x80000000;
	const uint32_t FLOAT_EXP_MASK = 0xff << 23;
	const uint32_t FLOAT_VAL_MASK = 0x7fffff;

	template<typename T>
	T lerp(T x, T y, float a)
	{
		return x + (y - x) * a;
	}

	static void printBits32(void *bits, std::string info = "")
	{
		uint32_t v = *(uint32_t*)bits;
		for (uint32_t i = 0x80000000; i; i >>= 1)
			std::cout << (i & v ? 1 : 0);
		std::cout << info;
	}

	static void printVec3(const glm::vec3 &v, std::string info = "")
	{
		std::cout << std::setprecision(6) << info << ":  " << v.x << "  " << v.y << "  " << v.z << std::endl;
	}

	static bool isNan(float v)
	{
		uint32_t u = *(int*)&v;
		return ((u & FLOAT_EXP_MASK) == FLOAT_EXP_MASK && (u & FLOAT_VAL_MASK) != 0);
	}

	inline static bool hasNan(const glm::vec3 &v)
	{
		return isNan(v.x) || isNan(v.y) || isNan(v.z);
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

	inline static uint32_t inverseBits(uint32_t bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
    	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return bits;
	}

	inline static float radicalInverseVDC(uint32_t bits)
	{
    	return float(inverseBits(bits)) * 2.3283064365386963e-10;
	}

	inline static glm::vec2 hammersley(uint32_t i, uint32_t n)
	{
		return glm::vec2((float)i / (float)n, radicalInverseVDC(i));
	}

	inline static bool refract(glm::vec3& Wt, const glm::vec3& Wi, const glm::vec3 &N, float eta)
	{
		// 与PBRT不同，这个的结果只与光路上折射率的比值eta有关，与N的取向无关
		float cosTi = glm::dot(N, Wi);
		float sin2Ti = 1.0f - cosTi * cosTi;
		float sin2Tt = eta * eta * sin2Ti;

		if (sin2Tt >= 1.0f) return false;

		float dirN = cosTi < 0 ? -1.0f : 1.0f;
		float cosTt = glm::sqrt(1.0f - sin2Tt) * dirN;
		Wt = glm::normalize(-Wi * eta + N * (eta * cosTi - cosTt));
		return true;
	}

	inline static float fresnelDieletric(float cosTi, float etaI, float etaT)
	{
		cosTi = glm::clamp(cosTi, -1.0f, 1.0f);
		float sinTi = glm::sqrt(1 - cosTi * cosTi);
		float sinTt = etaI / etaT * sinTi;
		if (sinTt >= 1.0f) return 1.0f;
		
		float cosTt = glm::sqrt(1.0f - sinTt * sinTt);

		float rPa = (etaT * cosTi - etaI * cosTt) / (etaT * cosTi + etaI * cosTt);
		float rPe = (etaI * cosTi - etaT * cosTt) / (etaI * cosTi + etaT * cosTt);

		float res = (rPa * rPa + rPe * rPe) * 0.5f;
		return res;
	}

	inline static glm::vec3 fresnelSchlick(float cosTheta, const glm::vec3 &F0)
	{
		return F0 + (glm::vec3(1.0) - F0) * (float)glm::pow(1.0 - cosTheta, 5.0);
	}

	inline static glm::vec3 fresnelSchlickRoughness(float cosTheta, const glm::vec3 &F0, float roughness)
	{
		return F0 + (glm::max(glm::vec3(1.0 - roughness), F0) - F0) * (float)glm::pow(1.0f - cosTheta, 5.0f);
	}

	inline static float distributionGGX(float NoH, float a)
	{
		float a2 = a * a;
		float denom = NoH * NoH * (a2 - 1.0f) + 1.0f + 1e-6f;
		denom = denom * denom * glm::pi<float>();

		return a2 / denom;
	}

	inline static float distributionGGX(const glm::vec3 &N, const glm::vec3 &H, float roughness)
	{
		float NdotH = std::max(glm::dot(N, H), 0.0f);
		return distributionGGX(NdotH, roughness);
	}

	inline static float pdfGGX(float NoH, float HoV, float a)
	{
		return distributionGGX(NoH, a) * NoH / (4.0 * HoV + 1e-6f);
	}

	inline static float geometrySchlickGGX(float NdotV, float roughness)
	{
		float r = roughness + 1.0f;
		float k = roughness * roughness / 2.0f;

		float nom = NdotV;
		float denom = NdotV * (1.0f - k) + k;

		return nom / denom;
	}

	inline static float geometrySmith(const glm::vec3 &N, const glm::vec3 &V, const glm::vec3 &L, float roughness)
	{
		float NdotV = glm::max(glm::dot(N, V), 0.0f);
		float NdotL = glm::max(glm::dot(N, L), 0.0f);

		float ggx2 = geometrySchlickGGX(NdotV, roughness);
		float ggx1 = geometrySchlickGGX(NdotL, roughness);

		return ggx1 * ggx2;
	}
}

#endif
