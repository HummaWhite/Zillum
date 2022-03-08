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

    Ray ray;
    Vec3f wo;
    Spectrum throughput;

    if (lightSource.index() == 0)
    {
        auto areaLight = std::get<0>(lightSource);

        Vec3f pLit = areaLight->uniformSample(sampler->get2());
        auto ciSamp = mScene->mCamera->sampleIi(pLit, sampler->get2());
        if (ciSamp)
        {
            auto [wi, Ii, dist, uvRaster, pdfIi] = ciSamp.value();
            Vec3f pCam = pLit + wi * dist;
            float pdfPos = 1.0f / areaLight->surfaceArea();
            if (mScene->visible(pLit, pCam))
            {
                auto Le = areaLight->Le({ pLit, wi });
                auto contrib = Le * Ii / (pdfIi * pdfPos* pdfSource);
                if (!Math::isBlack(contrib))
                    addToFilmLocked(uvRaster, contrib);
            }
        }

        auto leSamp = areaLight->sampleLe(sampler->get<4>());
        Vec3f nl = areaLight->normalGeom(leSamp.ray.ori);

        wo = -leSamp.ray.dir;
        ray = leSamp.ray.offset();
        throughput = leSamp.Le * Math::absDot(nl, -wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);
    }
    else
    {
        auto envLight = std::get<1>(lightSource);
        auto [emiRay, Le, pdf] = mScene->sampleLeEnv(sampler->get<6>());

        wo = -emiRay.dir;
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

        Vec3f pos = ray.get(distObj);
        auto surf = obj->surfaceInfo(pos);

        if (glm::dot(surf.ns, wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        if (!deltaBsdf)
        {
            auto directSample = mScene->mCamera->sampleIi(pos, sampler->get2());
            if (directSample)
            {
                auto [wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pos + wi * dist;
                if (mScene->visible(pos, pCam))
                {
                    Spectrum res = Ii * surf.material->bsdf(surf.ns, wo, wi, TransportMode::Importance) *
                        throughput * Math::satDot(surf.ns, wi) / pdf;
                    if (!Math::hasNan(res) && !Math::isNan(pdf) && pdf > 1e-8f && !Math::isBlack(res))
                        addToFilmLocked(uvRaster, res);
                }
            }
        }

        auto sample = surf.material->sample(surf.ns, wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = sample.value();

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

        float cosWi = deltaBsdf ? 1.0f : Math::satDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * cosWi / bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;
    }
}