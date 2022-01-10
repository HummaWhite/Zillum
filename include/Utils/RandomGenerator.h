#pragma once

#include <iostream>
#include <random>
#include <ctime>

typedef std::uniform_real_distribution<double> UniformDouble;
typedef std::uniform_real_distribution<float> UniformFloat;
typedef std::uniform_int_distribution<uint32_t> UniformUint;
typedef std::uniform_int_distribution<int> UniformInt;
typedef std::uniform_int_distribution<uint16_t> UniformUint16;
typedef std::uniform_int_distribution<int16_t> UniformInt16;

static std::default_random_engine globalRandomEngine(time(0));

static float uniformFloat()
{
	return UniformFloat(0.0f, 1.0f)(globalRandomEngine);
}

static float uniformFloat(float tMin, float tMax)
{
	return UniformFloat(tMin, tMax)(globalRandomEngine);
}

template<typename T>
T uniformInt(T tMin, T tMax)
{
	return std::uniform_int_distribution<T>(tMin, tMax)(globalRandomEngine);
}
