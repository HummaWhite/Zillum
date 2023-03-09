#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <tuple>
#include <cmath>

#include "glmIncluder.h"
#include "Utils/NamespaceDecl.h"
#include "Utils/RandomGenerator.h"
#include "Spectrum.h"

NAMESPACE_BEGIN(Math)

const float Pi = glm::pi<float>();
const float PiInv = 1.0f / glm::pi<float>();

const float OneMinusEpsilon = 0x1.fffffep-1;

template<typename T>
T pow5(const T &v) {
    T v2 = v * v;
    return v2 * v2 * v;
}

template <typename T>
T lerp(T x, T y, float a) {
	return x + (y - x) * a;
}

template <typename T>
void print(T val, int precision, std::string info = "\n") {
	std::cout << std::setprecision(precision) << val << info;
}

template <typename T>
float vecElement(const T &v, int dim) {
	return *((float *)(glm::value_ptr(v)) + dim);
}

template <typename T>
T square(T v) {
	return v * v;
}

template <typename T>
float lengthSquare(const T &v) {
	return glm::dot(v, v);
}

template <typename T>
float distSquare(const T &x, const T &y) {
	return lengthSquare(x - y);
}

void printBits32(void *bits, std::string info = "");
void printVec3(const Vec3f &v, std::string info = "");
std::string vec3ToString(const Vec3f &v);

bool isNan(float v);
bool hasNan(const Vec3f &v);
bool isInf(float v);

glm::mat3 TBNMatrix(const Vec3f &N);

uint32_t inverseBits(uint32_t bits);
float radicalInverse(uint32_t bits);

float satDot(const Vec3f &a, const Vec3f &b);
float absDot(const Vec3f &a, const Vec3f &b);

bool coin(float u);

bool sameHemisphere(const Vec3f &N, const Vec3f &A, const Vec3f &B);

float maxComponent(const Vec3f &v);
float minComponent(const Vec3f &v);
int maxExtent(const Vec3f &v);
int cubeMapFace(const Vec3f &dir);

float qpow(float x, int n);

float heuristic(int nf, float pf, int ng, float pg, int pow);
float biHeuristic(float pf, float pg);

float luminance(const Spectrum &c);
bool isBlack(const Spectrum &c);

float diskArea(float radius);

float mollify(const Vec3f &n, const Vec3f &wo, const Vec3f &wi, float dist, float radius);

NAMESPACE_END(Math)