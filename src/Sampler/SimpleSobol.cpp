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

float SimpleSobolSampler::get1() {
    float r = static_cast<float>(sobolSample(index, dim++, scramble)) * 0x1p-32f;
    if (randomScrambling) {
        scramble = rng();
    }
    return std::min(r, Math::OneMinusEpsilon);
}

void SimpleSobolSampler::setPixel(int x, int y) {
    dim = 0;
    rng.seed(y << 16 | x);
    scramble = rng();
}

void SimpleSobolSampler::nextSample() {
    index++;
    dim = 0;
}

void SimpleSobolSampler::nextSamples(size_t samples) {
    index += samples;
    dim = 0;
}

SamplerPtr SimpleSobolSampler::copy() {
    SimpleSobolSampler *sampler = new SimpleSobolSampler(*this);
    return SamplerPtr(sampler);
}