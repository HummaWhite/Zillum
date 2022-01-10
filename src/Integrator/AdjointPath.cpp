#include "Integrator.h"

Vec3f AdjointPathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, hit] = scene->closestHit(ray);
    if (hit == nullptr)
        return scene->env->getRadiance(ray.dir);

    if (hit->getType() == HittableType::Light)
    {
        auto lt = dynamic_cast<Light*>(hit.get());
        return lt->Le({ ray.get(dist), -ray.dir });
    }

    auto obj = dynamic_cast<Object*>(hit.get());
    Vec3f P = ray.get(dist);
    ray = { P, -ray.dir };
    auto camVertex = findNonSpecularHit(ray, obj->surfaceInfo(P), sampler);
    if (!camVertex)
        return Vec3f(0.0f);
    return trace(camVertex.value(), sampler);
}

std::optional<AdjointPathIntegrator::Vertex> AdjointPathIntegrator::findNonSpecularHit(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler)
{
    Vec3f beta(1.0f);
    for (int bounce = 1; bounce <= maxCameraDepth; bounce++)
    {
        bool deltaBsdf = sInfo.mat->bxdf().isDelta();
        if (!deltaBsdf)
            return Vertex{ray.ori, sInfo.Ns, ray.dir, sInfo.mat, Vec3f(1.0f)};

        auto [Wi, pdf, type, eta] = sInfo.mat->getSample(sInfo.Ns, ray.dir, sampler->get1D(), sampler->get2D());
        Ray newRay(ray.ori + Wi * 1e-4f, Wi);
        auto [dist, hit] = scene->closestHit(newRay);
        if (hit == nullptr)
            return std::nullopt;
        if (hit->getType() == HittableType::Light)
            return std::nullopt;
        
        auto obj = dynamic_cast<Object*>(hit.get());
        float NoWi = deltaBsdf ? 1.0f : Math::satDot(sInfo.Ns, Wi);
        beta *= sInfo.mat->bsdf(sInfo.Ns, ray.dir, Wi, TransportMode::Importance) * NoWi / pdf;
        Vec3f y = newRay.get(dist);
        sInfo = obj->surfaceInfo(y);
        ray = { y, -Wi };
    }
    return std::nullopt;
}

Vec3f AdjointPathIntegrator::trace(Vertex v, SamplerPtr sampler)
{
    auto [light, pdfLt] = scene->sampleLightAndEnv(sampler->get2D(), sampler->get1D());
    if (light.index() == 1)
        return Vec3f(0.0f);

    Vec3f res(0.0f);
    auto lt = std::get<0>(light);

    // auto Pd = lt->uniformSample(sampler->get2D());
    // float pdfD = 1.0f / lt->surfaceArea();
    // Vec3f Wd = glm::normalize(Pd - v.P);
    // res += lt->Le({ Pd, -Wd }) * v.mat->bsdf({ v.Wo, Wd, v.N }) *
    //     scene->g(Pd, v.P, lt->surfaceNormal(Pd), v.N) * v.beta / (pdfD * pdfLt);
    // return res;

    auto leSamp = lt->sampleLe(sampler->get<6>());
    Vec3f beta = leSamp.Le / (leSamp.pdfPos * leSamp.pdfDir * pdfLt);
    auto ray = leSamp.ray;
    for (int bounce = 1; bounce <= maxLightDepth; bounce++)
    {
        auto [dist, hit] = scene->closestHit(ray);
        if (hit == nullptr)
            break;
        if (hit->getType() != HittableType::Object)
            break;
        
        auto obj = dynamic_cast<Object*>(hit.get());
        Vec3f P = ray.get(dist);
        auto sInfo = obj->surfaceInfo(P);
        bool deltaBsdf = sInfo.mat->bxdf().isDelta();

        if (!deltaBsdf)
        {
            Vec3f Wi = glm::normalize(v.P - P);
            res += v.beta * v.mat->bsdf(v.N, -Wi, v.Wo, TransportMode::Importance) *
                scene->g(v.P, P, v.N, sInfo.Ns) * sInfo.mat->bsdf(sInfo.Ns, -ray.dir, Wi, TransportMode::Importance) *
                beta;
        }
        break;
        
        auto [bsSamp, bsdf] = sInfo.mat->sampleWithBsdf(sInfo.Ns, -ray.dir, sampler->get1D(), sampler->get2D(),
            TransportMode::Importance);
        float NoWi = deltaBsdf ? 1.0f : Math::satDot(sInfo.Ns, bsSamp.dir);
        beta *= bsdf * NoWi / bsSamp.pdf;

        ray = { P, bsSamp.dir };
    }
    return res;
}