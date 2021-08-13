#include "Path.h"

glm::vec3 PathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = scene->closestHit(ray);

    if (obj == nullptr)
        return scene->env->getRadiance(ray.dir);

    if (obj->type() == HittableType::Light)
    {
        auto lt = dynamic_cast<Light *>(obj.get());
        return lt->getRadiance(ray.get(dist), -ray.dir);
    }
    else
    {
        glm::vec3 p = ray.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        SurfaceInfo sInfo = ob->surfaceInfo(p);
        ray.ori = p;
        return trace(ray, sInfo, sampler);
    }
}

glm::vec3 PathIntegrator::trace(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler)
{
    glm::vec3 result(0.0f);
    glm::vec3 beta(1.0f);

    for (int bounce = 1; bounce <= tracingDepth; bounce++)
    {
        glm::vec3 P = ray.ori;
        glm::vec3 Wo = -ray.dir;
        glm::vec3 N = sInfo.norm;
        auto &mat = sInfo.material;

        bool deltaBsdf = sInfo.material->bxdf().isDelta();

        auto directIllumSample = sampler->get<5>();
        if (!deltaBsdf && sampleDirectLight)
        {
            auto [Wi, coef, samplePdf] = scene->sampleLightAndEnv(P, directIllumSample);
            if (samplePdf != 0.0f)
            {
                float bsdfPdf = mat->pdf(Wo, Wi, N);
                float weight = Math::biHeuristic(samplePdf, bsdfPdf);
                result += mat->bsdf({Wo, Wi, N}, 0) * beta * Math::satDot(N, Wi) * coef * weight;
            }
        }

        auto [sample, bsdf] = sInfo.material->sampleWithBsdf(N, Wo, sampler->get1D(), sampler->get2D());
        auto [Wi, bsdfPdf, type] = sample;

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        beta *= bsdf * NoWi / bsdfPdf;

        Ray newRay(P + Wi * 1e-4f, Wi);
        auto [dist, obj] = scene->closestHit(newRay);

        if (obj == nullptr)
        {
            float weight = 1.0f;
            if (!deltaBsdf && sampleDirectLight)
            {
                float envPdf = scene->env->pdfLi(Wi) * scene->pdfSelectEnv();
                weight = (envPdf <= 0.0f) ? 0.0f : Math::biHeuristic(bsdfPdf, envPdf);
            }
            result += scene->env->getRadiance(Wi) * envStrength * beta * weight;
            break;
        }

        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto lt = dynamic_cast<Light *>(obj.get());
            auto hitPoint = newRay.get(dist);
            if (!deltaBsdf && sampleDirectLight)
            {
                float lightPdf = lt->pdfLi(P, hitPoint) * scene->pdfSelectLight(lt);
                weight = (lightPdf <= 0.0f) ? 0.0f : Math::biHeuristic(bsdfPdf, lightPdf);
            }
            result += lt->getRadiance(hitPoint, -Wi) * beta * weight;
            break;
        }

        if (roulette && sampler->get1D() > rouletteProb)
            break;
        if (roulette)
            beta /= rouletteProb;

        glm::vec3 nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        sInfo = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
    }
    return result;
}