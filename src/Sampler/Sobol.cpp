#include "Core/Sampler.h"
#include "Utils/SobolMatrices1024x52.h"

uint32_t sobolSample(uint64_t index, int dim, uint32_t scramble = 0) {
    uint32_t r = scramble;
    for (int i = dim * SobolMatricesSize; index; index >>= 1, i++) {
        if (index & 1) {
            r ^= SobolMatrices[i];
        }
    }
    return r;
}

float SobolSampler::get1() {
    float r = static_cast<float>(sobolSample(index, dim++, scramble)) * 0x1p-32f;
    if (randomScrambling) {
        scramble = rng();
    }
    return std::min(r, Math::OneMinusEpsilon);
}

void SobolSampler::setPixel(int x, int y) {
    dim = 0;
    rng.seed(y << 16 | x);
    scramble = rng();
}

void SobolSampler::nextSample() {
    index++;
    dim = 0;
}

void SobolSampler::nextSamples(size_t samples) {
    index += samples;
    dim = 0;
}

SamplerPtr SobolSampler::copy() {
    SobolSampler *sampler = new SobolSampler(*this);
    return SamplerPtr(sampler);
}