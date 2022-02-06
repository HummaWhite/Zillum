#include "../../include/Core/Integrator.h"

Vec3f AOIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = mScene->closestHit(ray);
    if (obj == nullptr)
        return Vec3f(1.0f);

    auto p = ray.get(dist);
    ray.ori = p;
    return trace(ray, obj->normalGeom(p), sampler);
}

Vec3f AOIntegrator::trace(Ray ray, Vec3f N, SamplerPtr sampler)
{
    Vec3f ao(0.0f);

    for (int i = 0; i < mSamplesOneTime; i++)
    {
        auto Wi = Math::sampleHemisphereCosine(N, sampler->get2()).first;
        auto occRay = Ray(ray.ori, Wi).offset();
        if (mScene->quickIntersect(occRay, mRadius))
            ao += Vec3f(1.0f);
    }

    return Vec3f(1.0f) - ao / (float)mSamplesOneTime;
}