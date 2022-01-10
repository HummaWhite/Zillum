#include "Sampler.h"
#include "SobolMatrices1024x52.h"

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