#ifndef MATH_H
#define MATH_H

#include <iostream>
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

	inline glm::vec2 randBox()
	{
		RandomGenerator rg;
		return glm::vec2(rg.get(), rg.get());
	}

	inline glm::vec3 randHemisphere()
	{
		RandomGenerator rg;
		float phi = rg.get() * Pi * 2.0f;
		float theta = rg.get() * Pi * 0.5f;

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
}

#endif
