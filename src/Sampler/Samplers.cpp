#include "Samplers.h"

uint32_t sobolSample(uint64_t index, int dim, uint32_t scramble = 0)
{
    uint32_t r = scramble;
    for (int i = dim * SobolMatricesSize; index; index >>= 1, i++)
    {
        if (index & 1)
            r ^= SobolMatrices[i];
    }
    return r;
}

IndependentSampler::IndependentSampler() :
    Sampler(SamplerType::Independent)
{
    rng.seed(globalRandomEngine());
}

float IndependentSampler::get1D()
{
    return std::uniform_real_distribution<float>(0.0f, Math::OneMinusEpsilon)(rng);
}

Vec2f IndependentSampler::get2D()
{
    return Vec2f(get1D(), get1D());
}

SamplerPtr IndependentSampler::copy()
{
    IndependentSampler *sampler = new IndependentSampler(*this);
    return SamplerPtr(sampler);
}

float SimpleSobolSampler::get1D()
{
    float r = static_cast<float>(sobolSample(index, dim++, scramble)) * 0x1p-32f;
    scramble = rng();
    return std::min(r, Math::OneMinusEpsilon);
}

Vec2f SimpleSobolSampler::get2D()
{
    return Vec2f(get1D(), get1D());
}

void SimpleSobolSampler::setPixel(int x, int y)
{
    dim = 0;
    rng.seed(y * xPixels + x);
    scramble = rng();
}

SamplerPtr SimpleSobolSampler::copy()
{
    SimpleSobolSampler *sampler = new SimpleSobolSampler(*this);
    return SamplerPtr(sampler);
}