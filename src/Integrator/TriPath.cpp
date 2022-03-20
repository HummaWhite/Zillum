#include "../../include/Core/Integrator.h"

float weightS0(float s1s0, float t1s0)
{
    return 1.0f / (1.0f + s1s0 + t1s0);
}

float weightS1(float s1s0, float t1s1)
{
    return s1s0 / (1.0f + s1s0 + s1s0 * t1s1);
}

float weightT1(float s0t1, float s1t1)
{
    return 1.0f / (s0t1 + s1t1 + 1.0f);
}

float MISWeight(float a, float b, float c)
{
    return a / (a + b + c);
}

float pdfToArea(const Vec3f &fr, const Vec3f &to, const Vec3f &n, float pdfSolidAngle)
{
    float dist2 = Math::distSquare(fr, to);
    return pdfSolidAngle * Math::absDot(n, glm::normalize(fr - to)) / dist2;
}

float pdfToAngle(const Vec3f &fr, const Vec3f &to, const Vec3f &n, float pdfArea)
{
    float dist2 = Math::distSquare(fr, to);
    return pdfArea * dist2 / Math::absDot(n, glm::normalize(fr - to));
}

Spectrum traceCameraPath(const TriPathIntegParam &param, ScenePtr scene, Vec3f pos, Vec3f wo, SurfaceInfo surf,
                         Vec3f prevPos, Vec3f prevNorm, SamplerPtr sampler)
{
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    float s1s0 = 1.0f; // p(s=1) / p(s=0)
    float t1s0 = 1.0f; // p(t=1) / p(s=0)
    float t1s1 = t1s0; // p(t=1) / p(s=1)

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

        //auto lightSample = sampler->get<5>();
        if (!deltaBsdf)
        {
            auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
            if (lightSource.index() != 0)
                break;
            auto light = std::get<0>(lightSource);

            auto LiSample = light->sampleLi(pos, sampler->get2());
            if (!LiSample)
                break;

            Vec3f pLit = light->uniformSample(sampler->get2());
            if (scene->visible(pos, pLit))
            {
                Vec3f wi = glm::normalize(pLit - wi);
                Spectrum Le = light->Le({pLit, -wi});
                if (!Math::isBlack(Le))
                {
                    float bsdfPdf = mat->pdf(surf.ns, wo, wi);
                    if (bsdfPdf > 0)
                    {
                        float pdfPLit = pdfSource / light->surfaceArea();
                        float pdfToSurf = pdfToArea(pLit, pos, surf.ns, light->pdfLe({pLit, -wi}).pdfDir);
                        float pdfToLight = pdfToArea(pos, pLit, light->normalGeom(pLit), bsdfPdf);
                        float pdfToPrev = (bounce > 1) ? pdfToArea(pos, prevPos, prevNorm, mat->pdf(surf.ns, wi, wo, TransportMode::Importance)) : 1.0f;

                        float weight = weightS1(s1s0 * pdfPLit / pdfToLight, t1s1 * pdfToSurf * pdfToPrev);
                        if (weight > 1.0f)
                            weight = 0.0f;

                        result += Le * mat->bsdf(surf.ns, wo, wi, TransportMode::Radiance) *
                            throughput * Math::satDot(surf.ns, wi) * weightS1(s1s0 * pdfPLit / pdfToLight, t1s1 * pdfToSurf * pdfToPrev) /
                            light->pdfLi(pos, pLit);
                    }
                }
            }
        }

        auto bsdfSample = surf.material->sample(surf.ns, wo, sampler->get3());
        if (!bsdfSample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = bsdfSample.value();

        float cosWi = type.isDelta() ? 1.0f : Math::absDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf) || cosWi < 1e-6f)
            break;
        throughput *= bsdf * cosWi / bsdfPdf;

        auto nextRay = Ray(pos, wi).offset();
        auto [dist, obj] = scene->closestHit(nextRay);

        if (!obj)
        {
            // TODO: env light
            break;
        }
        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto light = dynamic_cast<Light *>(obj.get());
            Vec3f pLit = nextRay.get(dist);
            if (!deltaBsdf)
            {
                auto [pdfPos, pdfDir] = light->pdfLe({pLit, -wi});
                float pdfPLit = pdfPos * scene->pdfSampleLight(light);
                float pdfToLight = pdfToArea(pos, pLit, light->normalGeom(pLit), bsdfPdf);
                float pdfToSurf = pdfToArea(pLit, pos, surf.ns, pdfDir);

                if (pdfToLight < 1e-6f || Math::isNan(t1s0))
                    weight = 0;
                else
                    weight = weightS0(s1s0 * pdfPLit / pdfToLight, t1s0 * pdfToSurf * pdfPLit / pdfToLight);
            }
            result += light->Le({ pLit, -wi }) * throughput * weight;
            break;
        }

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= param.rrCameraStartDepth && param.rrCameraPath)
        {
            float continueProb = glm::min<float>(Math::maxComponent(bsdf / bsdfPdf) * etaScale, 0.95f);
            if (sampler->get1() >= continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.rrCameraPath)
            break;

        Vec3f nextPos = nextRay.get(dist);
        auto nextObj = dynamic_cast<Object *>(obj.get());
        auto nextSurf = nextObj->surfaceInfo(nextPos);

        float coef = 1.0f / pdfToArea(pos, nextPos, nextSurf.ns, bsdfPdf);
        if (bounce > 1)
            coef *= pdfToArea(pos, prevPos, prevNorm, mat->pdf(surf.ns, wi, wo, TransportMode::Importance));
        t1s0 *= coef;
        t1s1 *= coef;

        prevPos = pos;
        prevNorm = surf.ns;
        pos = nextPos;
        wo = -wi;
        surf = nextSurf;
    }
    return result;
}

void TriPathIntegrator::traceLightPath(SamplerPtr sampler)
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());

    Ray ray;
    Vec3f wo;
    Spectrum throughput;

    Vec3f prevPos;
    SurfaceInfo prevSurf;
    float prevPdfDir;

    if (lightSource.index() != 0)
        return;
    auto areaLight = std::get<0>(lightSource);

    Vec3f pLit = areaLight->uniformSample(sampler->get2());
    auto ciSamp = mScene->mCamera->sampleIi(pLit, sampler->get2());
    if (ciSamp)
    {
        auto [wi, Ii, dist, uvRaster, pdf] = ciSamp.value();
        Vec3f pCam = pLit + wi * dist;
        float pdfPos = 1.0f / areaLight->surfaceArea();
        if (mScene->visible(pLit, pCam))
        {
            auto Le = areaLight->Le({pLit, wi});
            auto contrib = Le;
            addToFilmLocked(uvRaster, contrib);
        }
    }

    auto leSamp = areaLight->sampleLe(sampler->get<4>());
    Vec3f nl = areaLight->normalGeom(leSamp.ray.ori);

    wo = -leSamp.ray.dir;
    ray = leSamp.ray.offset();
    throughput = leSamp.Le * Math::absDot(nl, -wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    prevPos = leSamp.ray.ori;
    prevSurf.ns = nl;
    prevPdfDir = leSamp.pdfDir;

    float s0t1 = 1.0f / (leSamp.pdfPos * pdfSource);
    float s1t1 = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object *>(hit.get());

        Vec3f pos = ray.get(distObj);
        auto surf = obj->surfaceInfo(pos);

        if (glm::dot(surf.ns, wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        float pdfToPos = pdfToArea(prevPos, pos, surf.ns, prevPdfDir);
        s0t1 /= pdfToPos;
        s1t1 /= pdfToPos;

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

                    float pdfToPrev = pdfToArea(pos, prevPos, prevSurf.ns,
                        surf.material->pdf(surf.ns, wi, wo, TransportMode::Radiance));
                    float pdfToSurf = pdfToArea(pCam, pos, surf.ns,
                        mScene->mCamera->pdfIe({ pCam, -wi }).pdfDir);

                    float s0t1Coef = pdfToPrev * pdfToSurf;
                    float s1t1Coef = pdfToSurf;
                    if (bounce > 1)
                        s1t1Coef *= pdfToPrev;

                    float weight = weightT1(s0t1 * s0t1Coef, s1t1 * s1t1Coef);
                    if (!Math::isNan(weight) && weight > 0)
                        addToFilmLocked(uvRaster, res * weight);
                }
            }
        }
        auto sample = surf.material->sample(surf.ns, wo, sampler->get3(), TransportMode::Importance);
        if (!sample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = sample.value();

        if (bounce >= mParam.rrLightStartDepth && mParam.rrLightPath)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            float rr = sampler->get1();
            if (rr > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= mParam.maxLightDepth && !mParam.rrLightPath)
            break;

        float cosWi = deltaBsdf ? 1.0f : Math::satDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;

        float pdfToPrev = pdfToArea(pos, prevPos, prevSurf.ns, surf.material->pdf(surf.ns, wi, wo, TransportMode::Radiance));
        s0t1 *= pdfToPrev;
        if (bounce > 1)
            s1t1 *= pdfToPrev;

        throughput *= bsdf * cosWi / bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;

        prevPos = pos;
        prevSurf = surf;
        prevPdfDir = bsdfPdf;
    }
}

void TriPathIntegrator::renderOnePass()
{
    if (mMaxSpp && mParam.spp >= mMaxSpp)
        return;
    auto &film = mScene->mCamera->film();
    int pathsOnePass = mPathsOnePass ? mPathsOnePass : film.width * film.height / MaxThreads;
    std::thread *threads = new std::thread[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(pathsOnePass * i * 2);
        threads[i] = std::thread(trace, this, pathsOnePass, threadSampler);
    }
    for (int i = 0; i < MaxThreads; i++)
        threads[i].join();
    delete[] threads;

    mSampler->nextSamples(pathsOnePass * MaxThreads * 2);

    mParam.spp += static_cast<float>(pathsOnePass) * MaxThreads / (film.width * film.height);
    mResultScale = 1.0f / mParam.spp;
    std::cout << "\r[TriPathIntegrator spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
}

void TriPathIntegrator::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mParam.spp = 0;
}

void TriPathIntegrator::trace(int paths, SamplerPtr sampler)
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
            result = traceCameraPath(mParam, mScene, pos, -ray.dir, surf, ray.ori, mScene->mCamera->f(), sampler);
        }
        addToFilmLocked(uv, result);
        sampler->nextSample();
    }
    for (int i = 0; i < paths; i++)
    {
        traceLightPath(sampler);
        sampler->nextSample();
    }
}