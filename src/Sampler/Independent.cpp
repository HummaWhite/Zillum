#include "../../include/Core/Sampler.h"

IndependentSampler::IndependentSampler() :
    Sampler(SamplerType::Independent)
{
    rng.seed(globalRandomEngine());
}

float IndependentSampler::get1()
{
    return std::uniform_real_distribution<float>(0.0f, Math::OneMinusEpsilon)(rng);
}

SamplerPtr IndependentSampler::copy()
{
    IndependentSampler *sampler = new IndependentSampler(*this);
    return SamplerPtr(sampler);
}