#include "Core/Integrator.h"

static float BSDFMollifyRadius = 1.0f;
int sppLPT = 0;
double accumTimeLPT = 0.0;

void LightPathIntegrator::renderOnePass()
{
    auto &film = mScene->mCamera->film();
    BSDFMollifyRadius = glm::pow(mResultScale, 0.1f);
    Timer timer;
    std::thread *threads = new std::thread[mThreads];
    for (int i = 0; i < mThreads; i++)
    {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(mPathsOnePass * i);
        threads[i] = std::thread(&LightPathIntegrator::trace, this, threadSampler);
    }
    for (int i = 0; i < mThreads; i++)
        threads[i].join();
    delete[] threads;

    accumTimeLPT += timer.get() / (mPathsOnePass * mThreads) * 1e9;

    mSampler->nextSamples(mPathsOnePass * mThreads);
    mPathCount += mPathsOnePass * mThreads;
    mResultScale = static_cast<float>(film.width) * film.height / mPathCount;
    std::cout << "\r[LightPathIntegrator paths: " << mPathCount << ", spp: " << std::fixed << std::setprecision(3) << 1.0f / mResultScale << "]";
    std::cout << accumTimeLPT / (++sppLPT);
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
    float correction = 1.0f;

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
                auto contrib = Le * Ii / (pdfIi * pdfPos * pdfSource);
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
            if (!surf.material->type().hasType(BSDFType::Transmission))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->type().isDelta();

        if (!deltaBsdf)
        {
            auto directSample = mScene->mCamera->sampleIi(pos, sampler->get2());
            if (directSample)
            {
                auto [wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pos + wi * dist;
                if (mScene->visible(pos, pCam))
                {
                    float cosWi = Math::satDot(surf.ng, wi) * glm::abs(glm::dot(surf.ns, wo) / glm::dot(surf.ng, wo));
                    Spectrum res = Ii * surf.material->bsdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Importance) *
                        throughput * cosWi / pdf;
                    // Vec3f bsdfCos = dynamic_cast<Mirror*>(surf.material.get()) ? Spectrum(Math::mollify(surf.ns, wo, wi, dist, BSDFMollifyRadius)) :
                    //     surf.material->bsdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Importance) * cosWi;
                    // Spectrum res = Ii * bsdfCos * throughput / pdf;
                    if (!Math::hasNan(res) && !Math::isNan(pdf) && pdf > 1e-8f && !Math::isBlack(res))
                        addToFilmLocked(uvRaster, res);
                }
            }
        }

        auto sample = surf.material->sample({ surf.ns, wo, surf.uv }, sampler->get3(), TransportMode::Importance);
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

        //float cosWi = type.isDelta() ? 1.0f : Math::satDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        float cosWi = type.isDelta() ? 1.0f : glm::abs(glm::dot(wi, surf.ng) * glm::dot(wo, surf.ns) / glm::dot(wo, surf.ng));
        throughput *= bsdf * cosWi / bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;
    }
}
