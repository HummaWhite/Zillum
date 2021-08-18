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
	const float Pi = glm::pi<float>();
	const float PiInv = 1.0f / glm::pi<float>();

	const float OneMinusEpsilon = 0x1.fffffep-1;

	const glm::vec3 BRIGHTNESS = glm::vec3(0.299f, 0.587f, 0.114f);

	template<typename T>
	T lerp(T x, T y, float a)
	{
		return x + (y - x) * a;
	}

	template<typename T>
	void print(T val, int precision, std::string info = "\n")
	{
		std::cout << std::setprecision(precision) << val << info;
	}

	template <typename T>
	float vecElement(const T &v, int dim)
	{
		return *((float*)(glm::value_ptr(v)) + dim);
	}

	template <typename T>
	T square(T v)
	{
		return v * v;
	}

	template <typename T>
	float lengthSquare(const T &v)
	{
		return glm::dot(v, v);
	}

	template <typename T>
	float distSquare(const T &x, const T &y)
	{
		return lengthSquare(x - y);
	}

	void printBits32(void *bits, std::string info = "");
	void printVec3(const glm::vec3 &v, std::string info = "");
	std::string vec3ToString(const glm::vec3 &v);

	bool isNan(float v);
	bool hasNan(const glm::vec3 &v);
	bool isInf(float v);

	glm::mat3 TBNMatrix(const glm::vec3 &N);

	uint32_t inverseBits(uint32_t bits);
	float radicalInverse(uint32_t bits);

	float satDot(const glm::vec3 &a, const glm::vec3 &b);
	float absDot(const glm::vec3 &a, const glm::vec3 &b);

	bool coin(float u);

	bool sameHemisphere(const glm::vec3 &N, const glm::vec3 &A, const glm::vec3 &B);

	float maxComponent(const glm::vec3 &v);
	float minComponent(const glm::vec3 &v);
	int maxExtent(const glm::vec3 &v);
	int cubeMapFace(const glm::vec3 &dir);

	float qpow(float x, int n);

	float heuristic(int nf, float pf, int ng, float pg, int pow);
	float biHeuristic(float pf, float pg);

	float rgbBrightness(const glm::vec3 &c);

	float diskArea(float radius);
}
