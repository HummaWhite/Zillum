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

std::mutex mtx;

void LightPathIntegrator::traceOnePath(SamplerPtr sampler)
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());

    Ray ray;
    Vec3f Wo;
    Spectrum throughput;

    if (lightSource.index() == 0)
    {
        auto areaLight = std::get<0>(lightSource);

        Vec3f Pd = areaLight->uniformSample(sampler->get2());
        auto ciSamp = mScene->mCamera->sampleIi(Pd, sampler->get2());
        if (ciSamp)
        {
            auto [Wi, Ii, dist, uvRaster, pdf] = ciSamp.value();
            Vec3f Pc = Pd + Wi * dist;
            Vec3f Nd = areaLight->normalGeom(Pd);
            float pdfPos = 1.0f / areaLight->surfaceArea();
            if (mScene->visible(Pd, Pc))
            {
                auto Le = areaLight->Le({ Pd, Wi });
                auto contrib = Le;
                addToFilmLocked(uvRaster, contrib);
            }
        }

        auto leSamp = areaLight->sampleLe(sampler->get<4>());
        Vec3f Nl = areaLight->normalGeom(leSamp.ray.ori);

        Wo = -leSamp.ray.dir;
        ray = leSamp.ray.offset();
        throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);
    }
    else
    {
        auto envLight = std::get<1>(lightSource);
        auto [emiRay, Le, pdf] = mScene->sampleLeEnv(sampler->get<6>());

        Wo = -emiRay.dir;
        ray = { emiRay.ori, emiRay.dir };
        throughput = Le / (pdfSource * pdf);
    }

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f P = ray.get(distObj);
        auto surf = obj->surfaceInfo(P);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        if (!deltaBsdf)
        {
            auto directSample = mScene->mCamera->sampleIi(P, sampler->get2());
            if (directSample)
            {
                auto [Wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = P + Wi * dist;
                if (mScene->visible(P, pCam))
                {
                    Spectrum res = Ii * surf.material->bsdf(surf.NShad, Wo, Wi, TransportMode::Importance) *
                        throughput * Math::satDot(surf.NShad, Wi) / pdf;
                    addToFilmLocked(uvRaster, res);
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
        ray = Ray(P, Wi).offset();
        Wo = -Wi;
    }
}