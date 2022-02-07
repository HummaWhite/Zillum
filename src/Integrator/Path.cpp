#include "../../include/Core/Integrator.h"

Spectrum PathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = mScene->closestHit(ray);

    if (obj == nullptr)
        return mScene->mEnv->radiance(ray.dir);

    if (obj->type() == HittableType::Light)
    {
        auto lt = dynamic_cast<Light*>(obj.get());
        auto y = ray.get(dist);
        return lt->Le({ y, -ray.dir });
    }
    else if (obj->type() == HittableType::Object)
    {
        Vec3f p = ray.get(dist);
        auto tmp = obj.get();
        auto ob = dynamic_cast<Object*>(obj.get());
        SurfaceInfo sInfo = ob->surfaceInfo(p);
        ray.ori = p;
        return trace(ray, sInfo, sampler);
    }
    Error::impossiblePath();
    return Spectrum(0.0f);
}

Spectrum PathIntegrator::trace(Ray ray, SurfaceInfo surf, SamplerPtr sampler)
{
    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; ; bounce++)
    {
        Vec3f P = ray.ori;
        Vec3f Wo = -ray.dir;
        Vec3f N = surf.NShad;
        MaterialPtr mat = surf.material;

        if (glm::dot(N, Wo) < 0)
        {
            auto bxdf = mat->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                N = -N;
        }
        bool deltaBsdf = mat->bxdf().isDelta();

        auto lightSample = sampler->get<5>();
        if (!deltaBsdf && mSampleLi)
        {
            auto [Wi, coef, lightPdf] = mScene->sampleLiLightAndEnv(P, lightSample);
            if (lightPdf != 0.0f)
            {
                float bsdfPdf = mat->pdf(N, Wo, Wi);
                float weight = mUseMIS ? Math::biHeuristic(lightPdf, bsdfPdf) : 0.5f;
                result += mat->bsdf(N, Wo, Wi, TransportMode::Radiance) * throughput * Math::satDot(N, Wi) * coef * weight;
            }
        }

        auto bsdfSample = surf.material->sample(N, Wo, sampler->get1(), sampler->get2());
        if (!bsdfSample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = bsdfSample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;

        auto newRay = Ray(P, Wi).offset();
        auto [dist, obj] = mScene->closestHit(newRay);

        if (obj == nullptr)
        {
            float weight = 1.0f;
            if (!deltaBsdf && mSampleLi)
            {
                float envPdf = mScene->mEnv->pdfLi(Wi) * mScene->pdfSampleEnv();
                weight = (envPdf <= 0.0f) ? 0.0f :
                    mUseMIS ? Math::biHeuristic(bsdfPdf, envPdf) : 0.5f;
            }
            result += mScene->mEnv->radiance(Wi) * throughput * weight;
            break;
        }

        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto lt = dynamic_cast<Light *>(obj.get());
            auto hitPoint = newRay.get(dist);
            if (!deltaBsdf && mSampleLi)
            {
                float lightPdf = lt->pdfLi(P, hitPoint) * mScene->pdfSampleLight(lt);
                weight = (lightPdf <= 0.0f) ? 0.0f :
                    mUseMIS ? Math::biHeuristic(bsdfPdf, lightPdf) : 0.5f;
            }
            result += lt->Le({ hitPoint, -Wi }) * throughput * weight;
            break;
        }

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= mRRStartDepth && mRussianRoulette)
        {
            float continueProb = glm::min<float>(0.9f, Math::maxComponent(bsdf / bsdfPdf) * etaScale);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= mMaxDepth && !mRussianRoulette)
            break;

        Vec3f nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        surf = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
    }
    return result;
}