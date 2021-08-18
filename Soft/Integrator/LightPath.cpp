#include "LightPath.h"

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
    scene->camera->getFilm().fill(glm::vec3(0.0f));
    pixelCount.fill(0);
    pathCount = 0;
}

void LightPathIntegrator::trace()
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(mSampler->get2D(), mSampler->get1D());

    if (lightSource.index() != 0)
        return;
    //Error::check(lightSource.index() == 0, "LightPath tracer currently only supports area light");

    auto u0s = mSampler->get<6>();
    auto leSamp = lightSource.index() == 0 ?
        std::get<0>(lightSource)->sampleLe(u0s) :
        std::get<1>(lightSource)->sampleLe(scene->boundRadius, u0s);

    if (lightSource.index() == 0)
    {
        auto lt = std::get<0>(lightSource);
        glm::vec3 Pl = leSamp.ray.ori;
        glm::vec3 NL = lt->surfaceNormal(Pl);
        auto [Wi, Ii, dist, uvRaster, pdfIi] = scene->camera->sampleIi(Pl, mSampler->get2D());
        
        if (pdfIi > 1e-8f)
        {
            glm::vec3 Pc = Pl + Wi * dist;
            if (!scene->occlude(Pl, Pc))
            {
                float pdfLi = lt->pdfLi(Pc, Pl);
                auto contrib = Ii * Math::absDot(NL, Wi) * leSamp.Le /
                    (pdfSource * pdfLi * pdfIi);

                //addToFilm(uvRaster, contrib);
            }
        }
    }

    auto lt = std::get<0>(lightSource);
    glm::vec3 Nl = lt->surfaceNormal(leSamp.ray.ori);
    glm::vec3 Wo = -leSamp.ray.dir;

    glm::vec3 beta = leSamp.Le * Math::satDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);
    Ray ray(leSamp.ray.ori - Wo * 1e-4f, -Wo);

    // Math::printVec3(beta, "Beta");
    // Math::printVec3(leSamp.Le, "Le");
    // Math::printVec3(Wo, "Wo");
    // std::cout << "\n";

    for (int bounce = 1; bounce <= maxDepth; bounce++)
    {
        auto [dHit, hit] = scene->closestHit(ray);
        if (hit == nullptr)
            break;
        if (hit->getType() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        glm::vec3 Ps = ray.get(dHit);
        auto sInfo = obj->surfaceInfo(Ps);
        bool deltaBsdf = sInfo.mat->bxdf().isDelta();

        if (bounce == 1)
            beta *= Math::satDot(sInfo.N, Wo) / (dHit * dHit);

        if (!deltaBsdf)
        {
            auto [Wi, Ii, dist, uvRaster, pdfIi] = scene->camera->sampleIi(Ps, mSampler->get2D());
            if (pdfIi != 0.0f)
            {
                glm::vec3 Pc = Ps + Wi * dist;
                if (!scene->occlude(Ps, Pc))
                {
                    auto res = Ii * sInfo.mat->bsdf({ Wo, Wi, sInfo.N }, TransportMode::Importance) *
                        beta * Math::absDot(sInfo.N, Wi) / pdfIi;
                    addToFilm(uvRaster, res * 40.0f);
                }
            }
        }
        auto [bsdfSample, bsdf] = sInfo.mat->sampleWithBsdf(sInfo.N, Wo, mSampler->get1D(), mSampler->get2D(),
            TransportMode::Importance);
        auto [Wi, bsdfPdf, type, eta] = bsdfSample;

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(sInfo.N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        beta *= bsdf * NoWi / bsdfPdf;
        ray = { Ps + Wi * 1e-4f, Wi };
        Wo = -Wi;
    }
}

void LightPathIntegrator::addToFilm(glm::vec2 uv, glm::vec3 val)
{
    if (!Camera::inFilmBound(uv))
        return;
    auto &spp = pixelCount(uv);
    auto &v = scene->camera->getFilm()(uv);
    v = (v * static_cast<float>(spp) + val) / static_cast<float>(spp + 1);
    spp++;
}