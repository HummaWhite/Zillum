#include "../../include/Core/Integrator.h"

void LightPathIntegrator::renderOnePass()
{
    for (int i = 0; i < pathsOnePass; i++)
    {
        trace();
        mSampler->nextSample();
    }
    pathCount += pathsOnePass;
    auto &film = scene->camera->getFilm();
    *resultScale = 2.0f * film.width * film.height / pathCount;
    std::cout << "\r[LightPathIntegrator paths: " << pathCount << ", spp: " << 0.5f / *resultScale;
}

void LightPathIntegrator::reset()
{
    scene->camera->getFilm().fill(Vec3f(0.0f));
    pathCount = 0;
}

void LightPathIntegrator::trace()
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(mSampler->get2D(), mSampler->get1D());

    if (lightSource.index() != 0)
        return;
    // Error::check(lightSource.index() == 0, "LightPath tracer currently only supports area light");
    auto lt = std::get<0>(lightSource);

    Vec3f Pd = lt->uniformSample(mSampler->get2D());
    auto ciSamp = scene->camera->sampleIi(Pd, mSampler->get2D());
    if (ciSamp)
    {
        auto [Wi, Ii, dist, uvRaster, pdf] = ciSamp.value();
        Vec3f Pc = Pd + Wi * dist;
        Vec3f Nd = lt->normalGeom(Pd);
        float pdfPos = 1.0f / lt->surfaceArea();
        if (scene->visible(Pd, Pc))
        {
            auto Le = lt->Le({ Pd, Wi });
            auto contrib = Le;
            addToFilm(uvRaster, contrib);
        }
    }

    auto leSamp = lt->sampleLe(mSampler->get<6>());
    Vec3f Nl = lt->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    Ray ray = leSamp.ray.offset();
    Vec3f beta = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1; bounce <= maxDepth; bounce++)
    {
        auto [distObj, hit] = scene->closestHit(ray);
        if (!hit)
            break;
        if (hit->getType() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f pShading = ray.get(distObj);
        auto surf = obj->surfaceInfo(pShading);
        bool deltaBsdf = surf.mat->bxdf().isDelta();

        if (!deltaBsdf)
        {
            auto directSample = scene->camera->sampleIi(pShading, mSampler->get2D());
            if (directSample)
            {
                auto [Wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pShading + Wi * dist;
                if (scene->visible(pShading, pCam))
                {
                    Vec3f res = Ii * surf.mat->bsdf(surf.Ns, Wo, Wi, TransportMode::Importance) *
                        beta * Math::satDot(surf.Ns, Wi) / pdf;
                    addToFilm(uvRaster, res);
                }
            }
        }
        auto sample = surf.mat->sample(surf.Ns, Wo, mSampler->get1D(), mSampler->get2D(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.Ns, Wi);
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
    auto &film = scene->camera->getFilm();
    int spp = pathCount / (film.width * film.height);
    film(uv.x, uv.y) += val;
}