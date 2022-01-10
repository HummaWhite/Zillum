#include "../../include/Core/Sampler.h"

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