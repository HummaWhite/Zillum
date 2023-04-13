#include "Core/Integrator.h"

Spectrum traceOnePath(const PathIntegParam &param, ScenePtr scene, Vec3f pos, Vec3f wo, SurfaceInfo surf, Sampler *sampler)
{
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        BSDFPtr mat = surf.bsdf;

        if (glm::dot(surf.ns, wo) <= 0)
        {
            if (!mat->type().hasType(BSDFType::Transmission))
                surf.flipNormal();
        }
        bool deltaBsdf = mat->type().isDelta();

        auto lightSample = sampler->get<5>();
        if (!deltaBsdf && param.sampleDirect)
        {
            auto [wi, coef, lightPdf] = scene->sampleLiLightAndEnv(pos, lightSample);
            if (lightPdf != 0)
            {
                SurfaceIntr intr(surf.ns, wo, wi, surf.uv, sampler);
                float bsdfPdf = mat->pdf(intr, TransportMode::Radiance);
                float weight = param.MIS ? Math::powerHeuristic(lightPdf, bsdfPdf) : param.directWeight;
                result += mat->bsdf(intr, TransportMode::Radiance) * throughput *
                    Math::absDot(surf.ns, wi) * coef * weight;
            }
        }

        auto bsdfSample = surf.bsdf->sample({ surf.ns, wo, surf.uv, sampler }, sampler->get3());
        if (!bsdfSample)
            break;
        auto [wi, bsdf, bsdfPdf, type, eta] = bsdfSample.value();

        float cosWi = type.isDelta() ? 1.0f : Math::absDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf) || cosWi < 1e-6f)
            break;
        throughput *= bsdf * cosWi / bsdfPdf;

        auto newRay = Ray(pos, wi).offset();
        auto [dist, obj] = scene->closestHit(newRay);

        if (scene->isLightOrEnv(obj)) {
            float weight = 1.f;
            auto hitPos = newRay.get(dist);

            if (!type.isDelta() && param.sampleDirect) {
                float lightPdf = scene->pdfL(obj, pos, hitPos, wi);
                weight = (lightPdf <= 0) ? 0 : (param.MIS ? Math::powerHeuristic(bsdfPdf, lightPdf) : (1. - param.directWeight));
            }
            result += scene->L(obj, pos, hitPos, wi) * throughput * weight;
            break;
        }

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= param.rrStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(Math::maxComponent(bsdf / bsdfPdf) * etaScale, 0.95f);
            if (sampler->get1() >= continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxDepth && !param.russianRoulette)
            break;

        pos = newRay.get(dist);
        wo = -wi;
        auto nextObj = dynamic_cast<Object*>(obj.get());
        surf = nextObj->surfaceInfo(pos);
    }
    return result;
}

Spectrum PathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = mScene->closestHit(ray);

    if (obj == nullptr)
        return mScene->mEnv->radiance(ray.dir);

    if (obj->type() == HittableType::Light)
    {
        auto light = dynamic_cast<Light*>(obj.get());
        auto pl = ray.get(dist);
        return light->Le({ pl, -ray.dir });
    }
    else if (obj->type() == HittableType::Object)
    {
        Vec3f pos = ray.get(dist);
        auto object = dynamic_cast<Object*>(obj.get());
        SurfaceInfo surf = object->surfaceInfo(pos);
        return traceOnePath(mParam, mScene, pos, -ray.dir, surf, sampler.get());
    }
    Error::impossiblePath();
    return Spectrum(0.0f);
}

double accumTime = 0.0;
int spp = 0;

void PathIntegrator2::renderOnePass()
{
    //BSDFMollifyRadius = glm::pow(1.0f + mParam.spp, -1.0f / 4.0f);
    if (mMaxSpp && mParam.spp >= mMaxSpp)
    {
        mFinished = true;
        return;
    }
    auto &film = mScene->mCamera->film();
    int pathsOnePass = mPathsOnePass ? mPathsOnePass : film.width * film.height / mThreads;
    std::thread *threads = new std::thread[mThreads];

    Timer timer;

    for (int i = 0; i < mThreads; i++)
    {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(pathsOnePass * i);
        threads[i] = std::thread(&PathIntegrator2::trace, this, pathsOnePass, threadSampler);
    }
    for (int i = 0; i < mThreads; i++)
        threads[i].join();
    delete[] threads;

    accumTime += timer.get() / (pathsOnePass * mThreads) * 1e9;

    mSampler->nextSamples(pathsOnePass * mThreads);
    mParam.spp += static_cast<float>(pathsOnePass) * mThreads / (film.width * film.height);
    mResultScale = 1.0f / mParam.spp;
    std::cout << "\r[PathIntegrator2 spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
    std::cout << accumTime / (++spp);
}

void PathIntegrator2::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mParam.spp = 0;
}

void PathIntegrator2::trace(int paths, SamplerPtr sampler)
{
    for (int i = 0; i < paths; i++)
    {
        Vec2f uv = sampler->get2();
        Ray ray = mScene->mCamera->generateRay(uv * Vec2f(2.0f, -2.0f) + Vec2f(-1.0f, 1.0f), sampler);
        auto [dist, obj] = mScene->closestHit(ray);
        Spectrum result;

        if (obj == nullptr)
            result = mScene->mEnv->radiance(ray.dir);
        else if (obj->type() == HittableType::Light)
        {
            auto light = dynamic_cast<Light*>(obj.get());
            auto pl = ray.get(dist);
            result = light->Le({ pl, -ray.dir });
        }
        else if (obj->type() == HittableType::Object)
        {
            Vec3f pos = ray.get(dist);
            auto object = dynamic_cast<Object*>(obj.get());
            SurfaceInfo surf = object->surfaceInfo(pos);
            result = traceOnePath(mParam, mScene, pos, -ray.dir, surf, sampler.get());
        }
        if (!Math::isBlack(result))
            addToFilmLocked(uv, result);
        sampler->nextSample();
    }
}