#include "../../include/Core/Integrator.h"

void LightPathIntegrator::renderOnePass()
{
    std::thread *threads = new std::thread[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(mPathsOnePass * i);
        threads[i] = std::thread(trace, this, threadSampler);
    }
    for (int i = 0; i < MaxThreads; i++)
        threads[i].join();
    delete[] threads;

    mSampler->nextSamples(mPathsOnePass * MaxThreads);
    mPathCount += mPathsOnePass * MaxThreads;
    auto &film = mScene->mCamera->film();
    mResultScale = static_cast<float>(film.width) * film.height / mPathCount;
    std::cout << "\r[LightPathIntegrator paths: " << mPathCount << ", spp: " << std::fixed << std::setprecision(3) << 1.0f / mResultScale << "]";
}

void LightPathIntegrator::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mPathCount = 0;
}

void LightPathIntegrator::trace(SamplerPtr sampler)
{
    for (int i = 0; i < mPathsOnePass; i++)
    {
        traceOnePath(sampler);
        sampler->nextSample();
    }
}

void LightPathIntegrator::traceOnePath(SamplerPtr sampler)
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());

    if (lightSource.index() != 0)
        return;
    auto lt = std::get<0>(lightSource);

    Vec3f Pd = lt->uniformSample(sampler->get2());
    auto ciSamp = mScene->mCamera->sampleIi(Pd, sampler->get2());
    if (ciSamp)
    {
        auto [Wi, Ii, dist, uvRaster, pdf] = ciSamp.value();
        Vec3f Pc = Pd + Wi * dist;
        Vec3f Nd = lt->normalGeom(Pd);
        float pdfPos = 1.0f / lt->surfaceArea();
        if (mScene->visible(Pd, Pc))
        {
            auto Le = lt->Le({ Pd, Wi });
            auto contrib = Le;
            addToFilm(uvRaster, contrib);
        }
    }

    auto leSamp = lt->sampleLe(sampler->get<4>());
    Vec3f Nl = lt->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    Ray ray = leSamp.ray.offset();
    Vec3f throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f pShading = ray.get(distObj);
        auto surf = obj->surfaceInfo(pShading);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        if (!deltaBsdf)
        {
            auto directSample = mScene->mCamera->sampleIi(pShading, sampler->get2());
            if (directSample)
            {
                auto [Wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pShading + Wi * dist;
                if (mScene->visible(pShading, pCam))
                {
                    Spectrum res = Ii * surf.material->bsdf(surf.NShad, Wo, Wi, TransportMode::Importance) *
                        throughput * Math::satDot(surf.NShad, Wi) / pdf;
                    addToFilm(uvRaster, res);
                }
            }
        }

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        if (bounce >= mRRStartDepth && mParam.russianRoulette)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            float rr = sampler->get1();
            if (rr > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= mParam.maxDepth && !mParam.russianRoulette)
            break;

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;
        ray = Ray(pShading, Wi).offset();
        Wo = -Wi;
    }
}

void LightPathIntegrator::addToFilm(Vec2f uv, Spectrum val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &film = mScene->mCamera->film();
    auto &filmLocker = mScene->mCamera->filmLocker()(uv.x, uv.y);
    filmLocker.lock();
    film(uv.x, uv.y) += val;
    filmLocker.unlock();
}