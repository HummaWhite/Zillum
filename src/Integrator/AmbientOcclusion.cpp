#include "../../include/Core/Integrator.h"

Spectrum traceOnePath(const AOIntegParam &param, ScenePtr scene, Ray ray, Vec3f N, SamplerPtr sampler)
{
    Spectrum ao(0.0f);
    for (int i = 0; i < param.samplesOneTime; i++)
    {
        auto Wi = Math::sampleHemisphereCosine(N, sampler->get2()).first;
        auto occRay = Ray(ray.ori, Wi).offset();
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

    auto p = ray.get(dist);
    ray.ori = p;
    return traceOnePath(mParam, mScene, ray, obj->normalGeom(p), sampler);
}

void AOIntegrator2::renderOnePass()
{
    auto &film = mScene->mCamera->film();
    int pathsOnePass = film.width * film.height / MaxThreads;
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
    mSpp += static_cast<float>(pathsOnePass) * MaxThreads / (film.width * film.height);
    mResultScale = 1.0f / mSpp;
    std::cout << "\r[AOIntegrator spp: " << mSpp << "]";
}

void AOIntegrator2::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mSpp = 0;
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
            Vec3f p = ray.get(dist);
            auto tmp = obj.get();
            auto ob = dynamic_cast<Object *>(obj.get());
            SurfaceInfo sInfo = ob->surfaceInfo(p);
            ray.ori = p;
            result = traceOnePath(mParam, mScene, ray, sInfo.NGeom, sampler);
        }
        addToFilm(uv, result);
        sampler->nextSample();
    }
}

void AOIntegrator2::addToFilm(Vec2f uv, Spectrum val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &film = mScene->mCamera->film();
    auto &filmLocker = mScene->mCamera->filmLocker()(uv.x, uv.y);
    filmLocker.lock();
    film(uv.x, uv.y) += val;
    filmLocker.unlock();
}