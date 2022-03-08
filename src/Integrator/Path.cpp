#include "../../include/Core/Integrator.h"

Spectrum traceOnePath(const PathIntegParam &param, ScenePtr scene, Vec3f pos, Vec3f wo, SurfaceInfo surf, SamplerPtr sampler)
{
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        MaterialPtr mat = surf.material;

        if (glm::dot(surf.ns, wo) <= 0)
        {
            auto bxdf = mat->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = mat->bxdf().isDelta();

        auto lightSample = sampler->get<5>();
        if (!deltaBsdf && param.sampleDirect)
        {
            auto [wi, coef, lightPdf] = scene->sampleLiLightAndEnv(pos, lightSample);
            if (lightPdf != 0)
            {
                float bsdfPdf = mat->pdf(surf.ns, wo, wi);
                float weight = param.MIS ? Math::biHeuristic(lightPdf, bsdfPdf) : 0.5f;
                result += mat->bsdf(surf.ns, wo, wi, TransportMode::Radiance) * throughput *
                    Math::satDot(surf.ns, wi) * coef * weight;
            }
        }

        auto bsdfSample = surf.material->sample(surf.ns, wo, sampler->get1(), sampler->get2());
        if (!bsdfSample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = bsdfSample.value();

        float cosWi = type.isDelta() ? 1.0f : Math::absDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf) || cosWi < 1e-6f)
            break;
        throughput *= bsdf * cosWi / bsdfPdf;

        auto newRay = Ray(pos, wi).offset();
        auto [dist, obj] = scene->closestHit(newRay);

        if (!obj)
        {
            float weight = 1.0f;
            if (!deltaBsdf && param.sampleDirect)
            {
                float envPdf = scene->mEnv->pdfLi(wi) * scene->pdfSampleEnv();
                weight = (envPdf <= 0) ? 0 : param.MIS ? Math::biHeuristic(bsdfPdf, envPdf) : 0.5f;
            }
            result += scene->mEnv->radiance(wi) * throughput * weight;
            break;
        }
        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto light = dynamic_cast<Light*>(obj.get());
            auto hitPoint = newRay.get(dist);
            if (!deltaBsdf && param.sampleDirect)
            {
                float lightPdf = light->pdfLi(pos, hitPoint) * scene->pdfSampleLight(light);
                weight = (lightPdf <= 0) ? 0 : param.MIS ? Math::biHeuristic(bsdfPdf, lightPdf) : 0.5f;
            }
            result += light->Le({ hitPoint, -wi }) * throughput * weight;
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
        return traceOnePath(mParam, mScene, pos, -ray.dir, surf, sampler);
    }
    Error::impossiblePath();
    return Spectrum(0.0f);
}

void PathIntegrator2::renderOnePass()
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
    std::cout << "\r[PathIntegrator2 spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
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
            result = traceOnePath(mParam, mScene, pos, -ray.dir, surf, sampler);
        }
        if (!Math::isBlack(result))
            addToFilmLocked(uv, result);
        sampler->nextSample();
    }
}