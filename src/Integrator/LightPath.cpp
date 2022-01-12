#include "../../include/Core/Integrator.h"

LightPathIntegrator::LightPathIntegrator(ScenePtr scene, int photonsOnePass) :
    pathsOnePass(photonsOnePass), Integrator(scene, IntegratorType::LightPath)
{
    auto film = scene->camera->getFilm();
    pixelCount.init(film.width, film.height);
    pixelCount.fill(0);
}

void LightPathIntegrator::renderOnePass()
{
    for (int i = 0; i < pathsOnePass; i++)
    {
        trace();
        mSampler->nextSample();
    }
    std::cout << "\r[LightPathIntegrator] Paths: " << pathCount;
    pathCount += pathsOnePass;
}

void LightPathIntegrator::reset()
{
    scene->camera->getFilm().fill(Vec3f(0.0f));
    pixelCount.fill(0);
    pathCount = 0;
}

void LightPathIntegrator::trace()
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(mSampler->get2D(), mSampler->get1D());

    if (lightSource.index() != 0)
        return;
    //Error::check(lightSource.index() == 0, "LightPath tracer currently only supports area light");
    auto lt = std::get<0>(lightSource);

    // Vec3f Pd = lt->uniformSample(mSampler->get2D());
    // auto ciSamp = scene->camera->sampleIi(Pd, mSampler->get2D());
    // if (ciSamp.pdf != 0.0f)
    // {
    //     Vec3f Pc = Pd + ciSamp.Wi * ciSamp.dist;
    //     Vec3f Nd = lt->surfaceNormal(Pd);
    //     float pdfPos = 1.0f / lt->surfaceArea();
    //     if (scene->visible(Pd, Pc))
    //     {
    //         auto Le = lt->Le({ Pd, ciSamp.Wi });
    //         auto contrib = Le;
    //         addToFilm(ciSamp.uv, contrib);
    //     }
    // }
    
    auto u0s = mSampler->get<6>();
    auto leSamp = lt->sampleLe(mSampler->get<6>());
    Vec3f Nl = lt->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    Ray ray(leSamp.ray.get(1e-4f), -Wo);
    Vec3f beta = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1; bounce <= maxDepth; bounce++)
    {
        auto [dHit, hit] = scene->closestHit(ray);
        if (hit == nullptr)
            break;
        if (hit->getType() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f Ps = ray.get(dHit);
        auto sInfo = obj->surfaceInfo(Ps);
        bool deltaBsdf = sInfo.mat->bxdf().isDelta();

        if (!deltaBsdf)
        {
            auto [Wi, Ii, dist, uvRaster, pdfIi] = scene->camera->sampleIi(Ps, mSampler->get2D());

            if (pdfIi != 0.0f)
            {
                Vec3f Pc = Ps + Wi * dist;
                if (scene->visible(Ps, Pc))
                {
                    auto res = Ii * sInfo.mat->bsdf(sInfo.Ns, Wo, Wi, TransportMode::Importance) *
                        beta * Math::satDot(sInfo.Ns, Wi) / pdfIi;
                    //addToFilm(uvRaster, res);
                    addToFilm(uvRaster, sInfo.mat->bsdf(sInfo.Ns, Wo, Wi, TransportMode::Importance) * beta / pdfIi);
                }
            }
        }
        auto sample = sInfo.mat->sample(sInfo.Ns, Wo, mSampler->get1D(), mSampler->get2D(),
            TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(sInfo.Ns, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        beta *= bsdf * NoWi / bsdfPdf;
        ray = { Ps + Wi * 1e-4f, Wi };
        Wo = -Wi;
    }
}

void LightPathIntegrator::addToFilm(Vec2f uv, Vec3f val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &spp = pixelCount(uv.x, uv.y);
    auto &v = scene->camera->getFilm()(uv.x, uv.y);
    v = (v * static_cast<float>(spp) + val) / static_cast<float>(spp + 1);
    spp++;
}