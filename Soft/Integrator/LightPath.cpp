#include "LightPath.h"

void LightPathIntegrator::renderOnePass()
{
    for (int i = 0; i < photonsOnePass; i++)
        trace();
}

void LightPathIntegrator::trace()
{
    glm::vec3 beta(1.0f);
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(mSampler->get2D(), mSampler->get1D());

    auto initSamples = mSampler->get<6>();

    auto [ray, Le, pdfLe] = (lightSource.index() == 0) ?
        std::get<0>(lightSource)->sampleLe(initSamples) :
        std::get<1>(lightSource)->sampleLe(scene->boundRadius, initSamples);

    for (int bounce = 1; bounce <= maxDepth; bounce++)
    {
    }
}