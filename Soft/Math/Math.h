#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <tuple>
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

	const float Pi = glm::pi<float>();
	const float PiInv = 1.0f / glm::pi<float>();

	const glm::vec3 BRIGHTNESS = glm::vec3(0.299f, 0.587f, 0.114f);

	template<typename T>
	T lerp(T x, T y, float a)
	{
		return x + (y - x) * a;
	}

	void printBits32(void *bits, std::string info = "")
	{
		uint32_t v = *(uint32_t*)bits;
		for (uint32_t i = 0x80000000; i; i >>= 1)
			std::cout << (i & v ? 1 : 0);
		std::cout << info;
	}

	template<typename T>
	void print(T val, int precision, std::string info = "\n")
	{
		std::cout << std::setprecision(precision) << val << info;
	}

	void printVec3(const glm::vec3 &v, std::string info = "")
	{
		std::cout << std::setprecision(6) << info << ":  " << v.x << "  " << v.y << "  " << v.z << std::endl;
	}

	std::string vec3ToString(const glm::vec3 &v)
	{
		std::stringstream ss;
		ss << "vec3{ x:" << v.x << ", y:" << v.y << ", z:" << v.z << " }";
		return ss.str();
	}

	template<typename T>
	inline float vecElement(const T &v, int dim)
	{
		return *((float*)(glm::value_ptr(v)) + dim);
	}

	inline bool isNan(float v)
	{
		uint32_t u = *(int*)&v;
		return ((u & FLOAT_EXP_MASK) == FLOAT_EXP_MASK && (u & FLOAT_VAL_MASK) != 0);
	}

	inline bool hasNan(const glm::vec3 &v)
	{
		return isNan(v.x) || isNan(v.y) || isNan(v.z);
	}

	inline bool isInf(float v)
	{
		uint32_t u = *(uint32_t*)&v;
		return ((u & FLOAT_EXP_MASK) == FLOAT_EXP_MASK && (u & FLOAT_VAL_MASK) == 0);
	}

	inline glm::mat3 TBNMatrix(const glm::vec3 &N)
	{
		glm::vec3 T = (glm::abs(N.z) > 0.99f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
		glm::vec3 B = glm::normalize(glm::cross(N, T));
		T = glm::normalize(glm::cross(B, N));
		return glm::mat3(T, B, N);
	}

	inline uint32_t inverseBits(uint32_t bits)
	{
		bits = (bits << 16u) | (bits >> 16u);
    	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
		return bits;
	}

	inline float radicalInverseVDC(uint32_t bits)
	{
    	return float(inverseBits(bits)) * 2.3283064365386963e-10;
	}

	inline glm::vec2 hammersley(uint32_t i, uint32_t n)
	{
		return glm::vec2((float)i / (float)n, radicalInverseVDC(i));
	}

	inline float satDot(const glm::vec3 &a, const glm::vec3 &b)
	{
		return glm::max(glm::dot(a, b), 0.0f);
	}

	inline float absDot(const glm::vec3 &a, const glm::vec3 &b)
	{
		return glm::abs(glm::dot(a, b));
	}

	inline bool coin()
	{
		return uniformFloat() < 0.5f;
	}

	inline glm::vec2 randBox()
	{
		return glm::vec2(uniformFloat(), uniformFloat());
	}

	inline glm::vec3 randHemisphere()
	{
		float phi = uniformFloat(0.0f, 2.0f * Pi);
		float theta = uniformFloat(0.0f, 0.5f * Pi);

		return glm::vec3(glm::cos(phi) * glm::sin(theta), glm::sin(phi) * glm::sin(theta), glm::cos(theta));
	}

	inline bool sameHemisphere(const glm::vec3 &N, const glm::vec3 &A, const glm::vec3 &B)
	{
		return glm::dot(N, A) * glm::dot(N, B) > 0.0f;
	}

	inline float maxComponent(const glm::vec3 &v)
	{
		return glm::max(glm::max(v.x, v.y), v.z);
	}

	inline float minComponent(const glm::vec3 &v)
	{
		return glm::min(glm::min(v.x, v.y), v.z);
	}

	inline int maxExtent(const glm::vec3 &v)
	{
		if (v.x > v.y) return v.x > v.z ? 0 : 2;
		return v.y > v.z ? 1 : 2;
	}

	inline int cubeMapFace(const glm::vec3 &dir)
	{
		int maxDim = maxExtent(glm::abs(dir));
		return maxDim * 2 + (vecElement(dir, maxDim) < 0);
	}

	inline float qpow(float x, int n)
	{
		float ret = 1.0f;
		while (n)
		{
			if (n & 1) ret *= x;
			x *= x, n >>= 1;
		}
		return ret;
	}

	inline float heuristic(int nf, float pf, int ng, float pg, int pow)
	{
		float f = qpow(nf * pf, pow);
		float g = qpow(ng * pg, pow);
		return f / (f + g);
	}

	inline float biHeuristic(float pf, float pg)
	{
		return heuristic(1, pf, 1, pg, 2);
	}

	template<typename T>
	T square(T v)
	{
		return v * v;
	}

	template <typename T>
	float lengthSquare(const T &v)
	{
		return square(glm::dot(v, v));
	}

	template<typename T>
	float distSquare(const T &x, const T &y)
	{
		return lengthSquare(x - y);
	}

	inline float rgbBrightness(const glm::vec3 &c)
	{
		return glm::dot(BRIGHTNESS, c);
	}
}
