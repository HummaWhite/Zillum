#pragma once

#include <random>

#include "Sampler.h"
#include "SobolMatrices1024x52.h"

class IndependentSampler :
    public Sampler
{
public:
    IndependentSampler();

    float get1D();
    glm::vec2 get2D();

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
		xPixels(xPixels), yPixels(yPixels) {}

    float get1D();
    glm::vec2 get2D();

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