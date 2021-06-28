#pragma once

#include <random>

#include "Sampler.h"
#include "SobolMatrices1024x52.h"

uint32_t sobolSample(uint64_t index, int dim, uint32_t scramble = 0)
{
    uint32_t r = scramble;
    for (int i = dim * SobolMatricesSize; index; index >>= 1, i++)
    {
        if (index & 1) r ^= SobolMatrices[i];
    }
    return r;
}

class IndependentSampler :
    public Sampler
{
public:
    IndependentSampler()
    {
        rng.seed(globalRandomEngine());
    }

    float get1D()
    {
        return std::uniform_real_distribution<float>(0.0f, Math::OneMinusEpsilon)(rng);
    }

    glm::vec2 get2D()
    {
        return glm::vec2(get1D(), get1D());
    }

    void setPixel(int x, int y) {}
    void nextSample() {}

    bool isProgressive() const { return true; }

    std::shared_ptr<Sampler> copy()
    {
        IndependentSampler *sampler = new IndependentSampler(*this);
        return std::shared_ptr<Sampler>(sampler);
    }

private:
    std::mt19937 rng;
};

class SimpleHaltonSampler;

class SimpleSobolSampler :
    public Sampler
{
public:
    SimpleSobolSampler(int xPixels, int yPixels) :
		xPixels(xPixels), yPixels(yPixels) {}

    float get1D()
    {
        float r = static_cast<float>(sobolSample(index, dim++, scramble)) * 0x1p-32f;
        scramble = rng();
        return std::min(r, Math::OneMinusEpsilon);
    }

    glm::vec2 get2D()
    {
        return glm::vec2(get1D(), get1D());
    }

    void setPixel(int x, int y)
    {
        dim = 0;
        rng.seed(y * xPixels + x);
        scramble = rng();
    }

    void nextSample()
    {
        index++;
    }

    bool isProgressive() const { return true; }

    std::shared_ptr<Sampler> copy()
    {
        SimpleSobolSampler *sampler = new SimpleSobolSampler(*this);
        return std::shared_ptr<Sampler>(sampler);
    }

private:
    const int xPixels, yPixels;
    uint64_t index = 0;
    int dim = 0;
    uint32_t scramble;
    std::mt19937 rng;
};
