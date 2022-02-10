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

	virtual float get1() = 0;
	virtual Vec2f get2() = 0;

	template<int N>
	std::array<float, N> get()
	{
		auto ret = std::array<float, N>();
		for (int i = 0; i < N; i++)
            ret[i] = get1();
		return ret;
	}

	virtual void setPixel(int x, int y) = 0;
	virtual void nextSample() = 0;
    virtual void nextSamples(size_t samples) {}
	virtual bool isProgressive() const = 0;
	virtual SamplerPtr copy() = 0;

	SamplerType getType() const { return type; }

private:
	SamplerType type;
};

class IndependentSampler :
    public Sampler
{
public:
    IndependentSampler();

    float get1();
    Vec2f get2();

    void setPixel(int x, int y) { rng.seed(globalRandomEngine()); }
    void nextSample() { rng.seed(globalRandomEngine()); }
    void nextSamples(size_t samples) override { rng.seed(globalRandomEngine()); }
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
    SimpleSobolSampler(int xPixels, int yPixels, bool randomScrambling) :
		xPixels(xPixels), yPixels(yPixels), randomScrambling(randomScrambling), Sampler(SamplerType::SimpleSobol) {}

    float get1();
    Vec2f get2();

    void setPixel(int x, int y);
    void nextSample();
    void nextSamples(size_t samples) override;
    bool isProgressive() const { return true; }
    SamplerPtr copy();

private:
    const int xPixels, yPixels;
    uint64_t index = 0;
    int dim = 0;
    bool randomScrambling;
    uint32_t scramble = 0;
    std::mt19937 rng;
};