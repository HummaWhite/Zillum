#include "Path.h"

glm::vec3 PathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = scene->closestHit(ray);

    if (obj == nullptr)
        return scene->env->getRadiance(ray.dir);

    if (obj->getType() == HittableType::Light)
    {
        auto lt = dynamic_cast<Light*>(obj.get());
        auto y = ray.get(dist);
        return lt->Le({ y, -ray.dir });
    }
    else if (obj->getType() == HittableType::Object)
    {
        glm::vec3 p = ray.get(dist);
        auto tmp = obj.get();
        auto ob = dynamic_cast<Object*>(obj.get());
        SurfaceInfo sInfo = ob->surfaceInfo(p);
        ray.ori = p;
        return trace(ray, sInfo, sampler);
    }
    Error::impossiblePath();
    return glm::vec3(0.0f);
}

glm::vec3 PathIntegrator::trace(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler)
{
    glm::vec3 result(0.0f);
    glm::vec3 beta(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; bounce <= tracingDepth; bounce++)
    {
        glm::vec3 P = ray.ori;
        glm::vec3 Wo = -ray.dir;
        glm::vec3 N = sInfo.N;
        auto &mat = sInfo.mat;

        bool deltaBsdf = sInfo.mat->bxdf().isDelta();

        auto directIllumSample = sampler->get<5>();
        if (!deltaBsdf && sampleDirectLight)
        {
            auto [Wi, coef, samplePdf] = scene->sampleLiLightAndEnv(P, directIllumSample);
            if (samplePdf != 0.0f)
            {
                float bsdfPdf = mat->pdf(Wo, Wi, N);
                float weight = Math::biHeuristic(samplePdf, bsdfPdf);
                result += mat->bsdf({Wo, Wi, N}, TransportMode::Radiance) * beta * Math::satDot(N, Wi) * coef * weight;
            }
        }

        auto [sample, bsdf] = sInfo.mat->sampleWithBsdf(N, Wo, sampler->get1D(), sampler->get2D());
        auto [Wi, bsdfPdf, type, eta] = sample;

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
                float envPdf = scene->env->pdfLi(Wi) * scene->pdfSampleEnv();
                weight = (envPdf <= 0.0f) ? 0.0f : Math::biHeuristic(bsdfPdf, envPdf);
            }
            result += scene->env->getRadiance(Wi) * envStrength * beta * weight;
            break;
        }

        if (obj->getType() == HittableType::Light)
        {
            float weight = 1.0f;
            auto lt = dynamic_cast<Light *>(obj.get());
            auto hitPoint = newRay.get(dist);
            if (!deltaBsdf && sampleDirectLight)
            {
                float lightPdf = lt->pdfLi(P, hitPoint) * scene->pdfSampleLight(lt);
                weight = (lightPdf <= 0.0f) ? 0.0f : Math::biHeuristic(bsdfPdf, lightPdf);
            }
            result += lt->Le({ hitPoint, -Wi }) * beta * weight;
            break;
        }

        if (obj->getType() != HittableType::Object)
        {
            std::cout << int(obj->getType()) << "\n";
        }

        if (sample.type.isTransmission())
            etaScale *= Math::square(eta);

        float rr = rouletteProb * etaScale;
        if (roulette && rr < 1.0f)
        {
            if (sampler->get1D() > rr)
                break;
            beta /= rr;
        }

        glm::vec3 nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        sInfo = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
    }
    return result;
}