#include "../../include/Core/Integrator.h"

Spectrum traceOnePath(const AOIntegParam &param, ScenePtr scene, Ray ray, Vec3f n, SamplerPtr sampler)
{
    Spectrum ao(0.0f);
    for (int i = 0; i < param.samplesOneTime; i++)
    {
        auto wi = Math::sampleHemisphereCosine(n, sampler->get2()).first;
        auto occRay = Ray(ray.ori, wi).offset();
        if (scene->quickIntersect(occRay, param.radius))
            ao += Spectrum(1.0f);
    }
    return Spectrum(1.0f) - ao / (float)param.samplesOneTime;
}

Spectrum AOIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = mScene->closestHit(ray);
    if (obj == nullptr)
        return Vec3f(1.0f);

    auto pos = ray.get(dist);
    ray.ori = pos;
    return traceOnePath(mParam, mScene, ray, obj->normalGeom(pos), sampler);
}

void AOIntegrator2::renderOnePass()
{
    if (mMaxSpp && mParam.spp >= mMaxSpp)
    {
        mFinished = true;
        return;
    }

    auto &film = mScene->mCamera->film();
    int pathsOnePass = mPathsOnePass ? mPathsOnePass : film.width * film.height / MaxThreads;
    std::thread *threads = new std::thread[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(pathsOnePass * i);
        threads[i] = std::thread(trace, this, pathsOnePass, threadSampler);
    }
    for (int i = 0; i < MaxThreads; i++)
        threads[i].join();
    delete[] threads;

    mSampler->nextSamples(pathsOnePass * MaxThreads);
    mParam.spp += static_cast<float>(pathsOnePass) * MaxThreads / (film.width * film.height);
    mResultScale = 1.0f / mParam.spp;
    std::cout << "\r[AOIntegrator2 spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
}

void AOIntegrator2::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mParam.spp = 0;
}

void AOIntegrator2::trace(int paths, SamplerPtr sampler)
{
    for (int i = 0; i < paths; i++)
    {
        Vec2f uv = sampler->get2();
        Ray ray = mScene->mCamera->generateRay(uv * Vec2f(2.0f, -2.0f) + Vec2f(-1.0f, 1.0f), sampler);
        auto [dist, obj] = mScene->closestHit(ray);

        Spectrum result(0.0f);
        if (obj == nullptr)
            result = mScene->mEnv->radiance(ray.dir);
        else if (obj->type() == HittableType::Object)
        {
            Vec3f pos = ray.get(dist);
            auto object = dynamic_cast<Object *>(obj.get());
            SurfaceInfo sInfo = object->surfaceInfo(pos);
            ray.ori = pos;
            result = traceOnePath(mParam, mScene, ray, sInfo.ng, sampler);
        }
        if (!Math::isBlack(result))
            addToFilmLocked(uv, result);
        sampler->nextSample();
    }
}