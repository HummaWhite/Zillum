#include "Core/Integrator.h"

inline float remap(float p) {
    return p < 1e-8f ? 1.0f : p * p;
}

float weightS0(float s1s0, float t1s0) {
    return 1.0f / (1.0f + s1s0 + t1s0);
}

float weightS1(float s1s0, float t1s1) {
    return s1s0 / (1.0f + s1s0 + s1s0 * t1s1);
}

float weightT1(float s0t1, float s1t1) {
    return 1.0f / (s0t1 + s1t1 + 1.0f);
}

float MISWeight(float a, float b, float c) {
    return a / (a + b + c);
}

float pdfToArea(const Vec3f &fr, const Vec3f &to, const Vec3f &n, float pdfSolidAngle) {
    float dist2 = Math::distSquare(fr, to);
    return pdfSolidAngle * Math::absDot(n, glm::normalize(fr - to)) / dist2;
}

float pdfToAngle(const Vec3f &fr, const Vec3f &to, const Vec3f &n, float pdfArea) {
    float dist2 = Math::distSquare(fr, to);
    return pdfArea * dist2 / Math::absDot(n, glm::normalize(fr - to));
}

Spectrum traceCameraPath(const TriplePathIntegParam &param, ScenePtr scene, Vec3f pos, Vec3f wo, SurfaceInfo surf,
    Vec3f prevPos, Vec3f prevNorm, SamplerPtr sampler, float primaryPdf
) {
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    float t1s0 = primaryPdf; // p(t=1) / p(s=0)
    float t1s1 = primaryPdf; // p(t=1) / p(s=1)

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++) {
        BSDFPtr mat = surf.bsdf;

        if (glm::dot(surf.ns, wo) <= 0) {
            if (!mat->type().hasType(BSDFType::Transmission)) {
                surf.flipNormal();
            }
        }
        bool deltaBsdf = mat->type().isDelta();

        //auto lightSample = sampler->get<5>();
        if (!deltaBsdf) {
            auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
            if (lightSource.index() != 0) {
                break;
            }
            auto light = std::get<0>(lightSource);

            auto LiSample = light->sampleLi(pos, sampler->get2());
            if (LiSample) {
                auto [wi, Le, dist, pdfLi] = LiSample.value();
                Vec3f pLit = pos + wi * dist;
                if (scene->visible(pos, pLit)) {
                    float pdfPLit = remap(pdfSource / light->surfaceArea());
                    float coefToSurf = remap(light->pdfLe({ pLit, -wi }).pdfDir * Math::absDot(surf.ns, wi));

                    float coefToLight = remap(mat->pdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Radiance) *
                        Math::satDot(light->normalGeom(pLit), -wi));

                    float coefToPrev = (bounce == 1) ? 1.0f :
                        remap(mat->pdf({ surf.ns, wi, wo, surf.uv }, TransportMode::Importance) * Math::absDot(prevNorm, wo));

                    float dist2 = remap(dist * dist);
                    float t1s0 = pdfPLit * dist2 / coefToLight;
                    float weight = weightS1(t1s0, t1s1 * coefToSurf * coefToPrev / dist2);

                    if (Math::isNan(weight) || weight > 1.0f || Math::isNan(t1s0) || t1s0 == 0) {
                        weight = 0;
                    }
                    
                    result += Le * mat->bsdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Radiance) * throughput * Math::absDot(surf.ns, wi) * weight /
                        (pdfLi * pdfSource);
                    // if (bounce == param.maxCameraDepth)
                    //     result = Spectrum(weight);
                }
            }
        }

        auto bsdfSample = surf.bsdf->sample({ surf.ns, wo, surf.uv }, sampler->get3());
        if (!bsdfSample) {
            break;
        }
        auto [wi, bsdf, bsdfPdf, type, eta] = bsdfSample.value();

        float cosWi = type.isDelta() ? 1.0f : Math::absDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf) || cosWi < 1e-6f) {
            break;
        }
        throughput *= bsdf * cosWi / bsdfPdf;

        auto nextRay = Ray(pos, wi).offset();
        auto [dist, obj] = scene->closestHit(nextRay);
        float pdfDirToNext = mat->pdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Radiance);
        float pdfDirToPrev = mat->pdf({ surf.ns, wi, wo, surf.uv }, TransportMode::Importance);
        // don't use bsdfPdf directly for MIS, we need to calculate again

        if (!obj) {
            // TODO: env light
            break;
        }
        if (obj->type() == HittableType::Light) {
            auto light = dynamic_cast<Light*>(obj.get());
            Vec3f pLit = nextRay.get(dist);
            auto [pdfPos, pdfDir] = light->pdfLe({ pLit, -wi });
            float pdfPLit = remap(pdfPos * scene->pdfSampleLight(light));

            float coefToLight = remap(pdfDirToNext * Math::satDot(light->normalGeom(pLit), -wi));
            float coefToSurf = remap(pdfDir * Math::absDot(surf.ns, wi));
            float coefToPrev = (bounce == 1) ? 1.0f : remap(pdfDirToPrev * Math::absDot(prevNorm, wo));

            float dist2 = remap(dist * dist);
            float weight = Math::isNan(t1s0) ? 0 : weightS0(pdfPLit * dist2 / coefToLight, t1s0 * coefToSurf * pdfPLit * coefToPrev / coefToLight);

            if (Math::isNan(weight) || weight > 1.0f) {
                weight = 0;
            }
            // if (bounce == param.maxCameraDepth)
            //     result = Spectrum(weight);
            //     result = light->Le({ pLit, -wi }) * throughput * weight;
            result += light->Le({ pLit, -wi }) * throughput * weight;
            break;
        }

        if (type.isTransmission()) {
            etaScale *= Math::square(eta);
        }

        if (bounce >= param.rrCameraStartDepth && param.rrCameraPath) {
            float continueProb = glm::min<float>(Math::maxComponent(bsdf / bsdfPdf) * etaScale, 0.95f);
            if (sampler->get1() >= continueProb) {
                break;
            }
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.rrCameraPath) {
            break;
        }

        Vec3f nextPos = nextRay.get(dist);
        auto nextObj = dynamic_cast<Object*>(obj.get());
        auto nextSurf = nextObj->surfaceInfo(nextPos);

        float coef = ((bounce == 1) ? 1.0f : remap(pdfDirToPrev * Math::absDot(prevNorm, wo))) /
            remap(pdfDirToNext * Math::absDot(nextSurf.ns, wi));
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

const int LPTtoPT = 1;

void TriplePathIntegrator::traceLightPath(SamplerPtr sampler) {
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());

    Ray ray;
    Vec3f wo;
    Spectrum throughput;

    Vec3f prevPos;
    SurfaceInfo prevSurf;
    float prevPdfDir;

    if (lightSource.index() != 0) {
        return;
    }
    auto areaLight = std::get<0>(lightSource);

    auto leSamp = areaLight->sampleLe(sampler->get<4>());
    Vec3f nl = areaLight->normalGeom(leSamp.ray.ori);

    wo = -leSamp.ray.dir;
    ray = leSamp.ray.offset();
    throughput = leSamp.Le * Math::absDot(nl, -wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    prevPos = leSamp.ray.ori;
    prevSurf.ns = nl;
    prevPdfDir = leSamp.pdfDir;

    float s0t1 = 1.0f / remap(leSamp.pdfPos * pdfSource);
    float s1t1 = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++) {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit) {
            break;
        }
        if (hit->type() != HittableType::Object) {
            break;
        }
        auto obj = dynamic_cast<Object *>(hit.get());

        Vec3f pos = ray.get(distObj);
        auto surf = obj->surfaceInfo(pos);

        if (glm::dot(surf.ns, wo) < 0) {
            if (!surf.bsdf->type().hasType(BSDFType::Transmission)) {
                surf.flipNormal();
            }
        }
        bool deltaBsdf = surf.bsdf->type().isDelta();

        float coefToPos = remap(prevPdfDir * Math::absDot(surf.ns, wo));
        s0t1 /= coefToPos;
        s1t1 /= coefToPos / (bounce == 1 ? remap(distObj * distObj) : 1.0f);

        if (!deltaBsdf) {
            auto directSample = mScene->mCamera->sampleIi(pos, sampler->get2());
            if (directSample) {
                auto [wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pos + wi * dist;

                if (mScene->visible(pos, pCam)) {
                    float cosWi = Math::satDot(surf.ng, wi) * glm::abs(glm::dot(surf.ns, wo) / glm::dot(surf.ng, wo));
                    Spectrum contrib = Ii * surf.bsdf->bsdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Importance) *
                        throughput * cosWi / pdf;

                    float coefToSurf = remap(mScene->mCamera->pdfIe({ pCam, -wi }).pdfDir * Math::satDot(surf.ns, wi));
                    float coefToPrev = remap(surf.bsdf->pdf({ surf.ns, wi, wo, surf.uv }, TransportMode::Radiance) *
                        Math::absDot(prevSurf.ns, wo));

                    float dist2 = remap(dist * dist);

                    float coef0 = coefToSurf * coefToPrev / dist2;
                    float coef1 = ((bounce == 1) ? 1.0f : coefToPrev) * coefToSurf / dist2;

                    float weight = weightT1(s0t1 * coef0, s1t1 * coef1);

                    if (Math::isNan(weight) || weight > 1.0f) {
                        weight = 0;
                    }
                    Spectrum res = contrib * weight;
                    if (!Math::isNan(weight) && weight > 0) {
                        //if (bounce == mParam.maxLightDepth)
                        addToFilmLocked(uvRaster, res * static_cast<float>(LPTtoPT));
                    }
                }
            }
        }
        auto sample = surf.bsdf->sample({ surf.ns, wo, surf.uv }, sampler->get3(), TransportMode::Importance);
        if (!sample) {
            break;
        }
        auto [wi, bsdf, bsdfPdf, type, eta] = sample.value();

        if (bounce >= mParam.rrLightStartDepth && mParam.rrLightPath) {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            float rr = sampler->get1();
            if (rr > continueProb) {
                break;
            }
            throughput /= continueProb;
        }
        if (bounce >= mParam.maxLightDepth && !mParam.rrLightPath) {
            break;
        }

        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf)) {
            break;
        }

        float coefToPrev = remap(surf.bsdf->pdf({ surf.ns, wi, wo, surf.uv }, TransportMode::Radiance) *
            Math::absDot(prevSurf.ns, wo));

        s0t1 *= coefToPrev;
        s1t1 *= (bounce == 1) ? 1.0f : coefToPrev;

        prevPdfDir = surf.bsdf->pdf({ surf.ns, wo, wi, surf.uv }, TransportMode::Importance);
        prevPos = pos;
        prevSurf = surf;

        float cosWi = type.isDelta() ? 1.0f : glm::abs(glm::dot(surf.ng, wi) * glm::dot(surf.ns, wo) / glm::dot(surf.ng, wo));
        throughput *= bsdf * cosWi / bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;
    }
}

double accumTimeTPT = 0.0;
int sppTPT = 0;

void TriplePathIntegrator::renderOnePass() {
    if (mMaxSpp && mParam.spp >= mMaxSpp) {
        return;
    }
    auto &film = mScene->mCamera->film();
    int pathsOnePass = mPathsOnePass ? mPathsOnePass : film.width * film.height / mThreads;
    std::thread *threads = new std::thread[mThreads];

    Timer timer;

    for (int i = 0; i < mThreads; i++) {
        auto threadSampler = mSampler->copy();
        threadSampler->nextSamples(pathsOnePass * i);
        threads[i] = std::thread(&TriplePathIntegrator::trace, this, pathsOnePass, threadSampler);
    }
    for (int i = 0; i < mThreads; i++) {
        threads[i].join();
    }
    delete[] threads;

    mSampler->nextSamples(pathsOnePass * mThreads);

    accumTimeTPT += timer.get() / (pathsOnePass * mThreads * 2) * 1e9;

    mParam.spp += static_cast<float>(pathsOnePass) * mThreads / (film.width * film.height);
    mResultScale = 1.0f / mParam.spp;
    std::cout << "\r[TriPathIntegrator spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
    std::cout << accumTimeTPT / (++sppTPT);
}

void TriplePathIntegrator::reset() {
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mParam.spp = 0;
}

void TriplePathIntegrator::trace(int paths, SamplerPtr sampler) {
    for (int i = 0; i < paths; i++) {
        Vec2f uv = sampler->get2();
        Ray ray = mScene->mCamera->generateRay(uv * Vec2f(2.0f, -2.0f) + Vec2f(-1.0f, 1.0f), sampler);
        auto [dist, obj] = mScene->closestHit(ray);
        Spectrum result(0.0f);

        if (obj == nullptr) {
            result = mScene->mEnv->radiance(ray.dir);
        }
        else if (obj->type() == HittableType::Light) {
            auto light = dynamic_cast<Light*>(obj.get());
            auto pl = ray.get(dist);
            result = light->Le({ pl, -ray.dir });
        }
        else if (obj->type() == HittableType::Object) {
            Vec3f pos = ray.get(dist);
            auto object = dynamic_cast<Object*>(obj.get());
            SurfaceInfo surf = object->surfaceInfo(pos);
            auto [pdfPos, pdfDir] = mScene->mCamera->pdfIe(ray);
            result = traceCameraPath(mParam, mScene, pos, -ray.dir, surf, ray.ori, mScene->mCamera->f(), sampler,
                remap(pdfPos) / remap(pdfToArea(ray.ori, pos, surf.ns, pdfDir)));
        }
        addToFilmLocked(uv, result);

        if (i % LPTtoPT == 0) {
            traceLightPath(sampler);
        }
        sampler->nextSample();
    }
}