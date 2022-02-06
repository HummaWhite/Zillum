#include "../../include/Core/Integrator.h"

void LightPathIntegrator::renderOnePass()
{
    for (int i = 0; i < mPathsOnePass; i++)
    {
        trace();
        //mSampler->nextSample();
    }
    mPathCount += mPathsOnePass;
    auto &film = mScene->mCamera->getFilm();
    *mResultScale = 2.0f * film.width * film.height / mPathCount;
    std::cout << "\r[LightPathIntegrator paths: " << mPathCount << ", spp: " << 0.5f / *mResultScale << "]";
}

void LightPathIntegrator::reset()
{
    mScene->mCamera->getFilm().fill(Vec3f(0.0f));
    mPathCount = 0;
}

void LightPathIntegrator::trace()
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(mSampler->get2(), mSampler->get1());

    if (lightSource.index() != 0)
        return;
    auto lt = std::get<0>(lightSource);

    Vec3f Pd = lt->uniformSample(mSampler->get2());
    auto ciSamp = mScene->mCamera->sampleIi(Pd, mSampler->get2());
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

    auto leSamp = lt->sampleLe(mSampler->get<4>());
    Vec3f Nl = lt->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    Ray ray = leSamp.ray.offset();
    Vec3f beta = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1; ; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->getType() != HittableType::Object)
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
            auto directSample = mScene->mCamera->sampleIi(pShading, mSampler->get2());
            if (directSample)
            {
                auto [Wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pShading + Wi * dist;
                if (mScene->visible(pShading, pCam))
                {
                    Vec3f res = Ii * surf.material->bsdf(surf.NShad, Wo, Wi, TransportMode::Importance) *
                        beta * Math::satDot(surf.NShad, Wi) / pdf;
                    addToFilm(uvRaster, res);
                }
            }
        }

        auto sample = surf.material->sample(surf.NShad, Wo, mSampler->get1(), mSampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        if (bounce >= mRRStartDepth && mRussianRoulette)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            float rr = mSampler->get1();
            if (rr > continueProb)
                break;
            beta /= continueProb;
        }
        if (bounce >= mMaxDepth && !mRussianRoulette)
            break;

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        beta *= bsdf * NoWi / bsdfPdf;
        ray = Ray(pShading, Wi).offset();
        Wo = -Wi;
    }
}

void LightPathIntegrator::addToFilm(Vec2f uv, Vec3f val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &film = mScene->mCamera->getFilm();
    int spp = mPathCount / (film.width * film.height);
    film(uv.x, uv.y) += val;
}