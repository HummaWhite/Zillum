#include "../../include/Core/Integrator.h"

Spectrum traceOnePath(const PathIntegParam &param, ScenePtr scene, Ray ray, SurfaceInfo surf, SamplerPtr sampler)
{
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        Vec3f P = ray.ori;
        Vec3f Wo = -ray.dir;
        Vec3f N = surf.NShad;
        MaterialPtr mat = surf.material;

        if (glm::dot(N, Wo) < 0)
        {
            auto bxdf = mat->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                N = -N;
        }
        bool deltaBsdf = mat->bxdf().isDelta();

        auto lightSample = sampler->get<5>();
        if (!deltaBsdf && param.sampleDirect)
        {
            auto [Wi, coef, lightPdf] = scene->sampleLiLightAndEnv(P, lightSample);
            if (lightPdf != 0.0f)
            {
                float bsdfPdf = mat->pdf(N, Wo, Wi);
                float weight = param.MIS ? Math::biHeuristic(lightPdf, bsdfPdf) : 0.5f;
                result += mat->bsdf(N, Wo, Wi, TransportMode::Radiance) * throughput * Math::satDot(N, Wi) * coef * weight;
            }
        }

        auto bsdfSample = surf.material->sample(N, Wo, sampler->get1(), sampler->get2());
        if (!bsdfSample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = bsdfSample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;

        auto newRay = Ray(P, Wi).offset();
        auto [dist, obj] = scene->closestHit(newRay);

        if (!obj)
        {
            float weight = 1.0f;
            if (!deltaBsdf && param.sampleDirect)
            {
                float envPdf = scene->mEnv->pdfLi(Wi) * scene->pdfSampleEnv();
                weight = (envPdf <= 0.0f) ? 0.0f : param.MIS ? Math::biHeuristic(bsdfPdf, envPdf)
                                                           : 0.5f;
            }
            result += scene->mEnv->radiance(Wi) * throughput * weight;
            break;
        }
        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto lt = dynamic_cast<Light *>(obj.get());
            auto hitPoint = newRay.get(dist);
            if (!deltaBsdf && param.sampleDirect)
            {
                float lightPdf = lt->pdfLi(P, hitPoint) * scene->pdfSampleLight(lt);
                weight = (lightPdf <= 0.0f) ? 0.0f : param.MIS ? Math::biHeuristic(bsdfPdf, lightPdf)
                                                             : 0.5f;
            }
            result += lt->Le({ hitPoint, -Wi }) * throughput * weight;
            break;
        }

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= param.rrStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(0.9f, Math::maxComponent(bsdf / bsdfPdf) * etaScale);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxDepth && !param.russianRoulette)
            break;

        Vec3f nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        surf = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
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
        auto lt = dynamic_cast<Light *>(obj.get());
        auto y = ray.get(dist);
        return lt->Le({ y, -ray.dir });
    }
    else if (obj->type() == HittableType::Object)
    {
        Vec3f p = ray.get(dist);
        auto ob = dynamic_cast<Object*>(obj.get());
        SurfaceInfo surf = ob->surfaceInfo(p);
        ray.ori = p;
        return traceOnePath(mParam, mScene, ray, surf, sampler);
    }
    Error::impossiblePath();
    return Spectrum(0.0f);
}

void PathIntegrator2::renderOnePass()
{
    if (mMaxSpp && mParam.spp >= mMaxSpp)
        return;
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
            auto lt = dynamic_cast<Light *>(obj.get());
            auto y = ray.get(dist);
            result = lt->Le({ y, -ray.dir });
        }
        else if (obj->type() == HittableType::Object)
        {
            Vec3f p = ray.get(dist);
            auto tmp = obj.get();
            auto ob = dynamic_cast<Object *>(obj.get());
            SurfaceInfo surf = ob->surfaceInfo(p);
            ray.ori = p;
            result = traceOnePath(mParam, mScene, ray, surf, sampler);
        }
        addToFilmLocked(uv, result);
        sampler->nextSample();
    }
}