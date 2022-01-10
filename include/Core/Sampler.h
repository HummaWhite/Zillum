#pragma once

#include <iostream>
#include <random>
#include <memory>
#include <array>

#include "../../ext/glmIncluder.h"
#include "Math.h"

enum class SamplerType
{
	Independent, SimpleSobol
};

class Sampler;
using SamplerPtr = std::shared_ptr<Sampler>;

class Sampler
{
public:
	Sampler(SamplerType type) : type(type) {}

	virtual float get1D() = 0;
	virtual Vec2f get2D() = 0;

	template<int N>
	std::array<float, N> get()
	{
		auto ret = std::array<float, N>();
		for (int i = 0; i < N; i++) ret[i] = get1D();
		return ret;
	}

	virtual void setPixel(int x, int y) = 0;
	virtual void nextSample() = 0;
	virtual bool isProgressive() const = 0;
	virtual std::shared_ptr<Sampler> copy() = 0;

	SamplerType getType() const { return type; }

private:
	SamplerType type;
};

class IndependentSampler :
    public Sampler
{
public:
    IndependentSampler();

    float get1D();
    Vec2f get2D();

    void setPixel(int x, int y) {}
    void nextSample() {}
    bool isProgressive() const { return true; }
    SamplerPtr copy();

private:
    std::mt19937 rng;
};

class SimpleHaltonSampler;

class SimpleSobolSampler :
    public Sampler
{
public:
    SimpleSobolSampler(int xPixels, int yPixels) :
		xPixels(xPixels), yPixels(yPixels), Sampler(SamplerType::SimpleSobol) {}

    float get1D();
    Vec2f get2D();

    void setPixel(int x, int y);
    void nextSample() { index++; }
    bool isProgressive() const { return true; }
    SamplerPtr copy();

private:
    const int xPixels, yPixels;
    uint64_t index = 0;
    int dim = 0;
    uint32_t scramble;
    std::mt19937 rng;
};