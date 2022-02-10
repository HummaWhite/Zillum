#include "../../include/Core/Integrator.h"

enum class VertexType
{
    EnvLight,
    AreaLight,
    Surface,
    Camera
};

struct BDPTVertex
{
    static BDPTVertex createCamera();

    static BDPTVertex createSurface();

    static BDPTVertex createEnvLight(const Vec3f &Wi, float pdf, Environment *env)
    {
        BDPTVertex v;
        v.W = Wi;
        v.pdfForw = pdf;
        v.envLight = env;
        return v;
    }

    static BDPTVertex createAreaLight(const Vec3f &P, const Vec3f &N, float pdf, Light *light)
    {
        BDPTVertex v;
        v.P = P;
        v.N = N;
        v.pdfForw = pdf;
        v.areaLight = light;
        v.type = VertexType::AreaLight;
        return v;
    }

    Vec3f P;
    Vec3f N;
    Vec3f W;

    float pdfForw;
    float pdfBack;
    Spectrum throughput;

    union
    {
        Environment *envLight;
        Light *areaLight;
        Material *surface;
        Camera *camera;
    };

    VertexType type;
};

struct VertexPath
{
	void add(const BDPTVertex &v)
	{
		vertices[length++] = v;
	}
	BDPTVertex vertices[TracingDepthLimit];
	int length = 0;
};

Spectrum BDPTIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    VertexPath lightPath, cameraPath;
    createLightPath(lightPath, mLightSampler);
    createCameraPath(cameraPath, ray, sampler);
    return eval(lightPath, cameraPath);
}

void BDPTIntegrator::createLightPath(VertexPath &path, SamplerPtr sampler)
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());
    if (lightSource.index() != 0)
        return;
    auto light = std::get<0>(lightSource);

    auto leSamp = light->sampleLe(sampler->get<4>());
    Vec3f Nl = light->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    path.add(BDPTVertex::createAreaLight(leSamp.ray.ori, Nl, pdfSource * leSamp.pdfPos, light.get()));

    Ray ray = leSamp.ray.offset();
    Vec3f throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
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
            auto directSample = mScene->mCamera->sampleIi(pShading, sampler->get2());
            if (directSample)
            {
                auto [Wi, Ii, dist, uvRaster, pdf] = directSample.value();
                Vec3f pCam = pShading + Wi * dist;
                if (mScene->visible(pShading, pCam))
                {
                    Spectrum res = Ii * surf.material->bsdf(surf.NShad, Wo, Wi, TransportMode::Importance) *
                        throughput * Math::satDot(surf.NShad, Wi) / pdf;
                    // addToFilm(uvRaster, res);
                }
            }
        }

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        if (bounce >= mParam.rrCameraStartDepth && mParam.russianRoulette)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            float rr = sampler->get1();
            if (rr > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= mParam.maxCameraDepth && !mParam.russianRoulette)
            break;

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;
        ray = Ray(pShading, Wi).offset();
        Wo = -Wi;
    }
}

void BDPTIntegrator::createCameraPath(VertexPath &path, Ray ray, SamplerPtr sampler)
{
    // path.add(BDPTVertex::createCamera(ray.ori, mCamera->f(), 1.0f, mCamera));

    Spectrum result(0.0f);
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
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

        auto bsdfSample = surf.material->sample(N, Wo, sampler->get1(), sampler->get2());
        if (!bsdfSample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = bsdfSample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;

        auto newRay = Ray(P, Wi).offset();
        auto [dist, obj] = scene->closestHit(newRay);

        if (obj == nullptr)
        {
            break;
        }

        if (obj->type() == HittableType::Light)
        {
            float weight = 1.0f;
            auto lt = dynamic_cast<Light *>(obj.get());
            auto hitPoint = newRay.get(dist);
            break;
        }

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= param.rrStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(0.9f, Math::maxComponent(bsdf / bsdfPdf) * etaScale);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxDepth && !param.russianRoulette)
            break;

        Vec3f nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        surf = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
    }
    return result;
}

Spectrum BDPTIntegrator::eval(const VertexPath &lightPath, const VertexPath &cameraPath)
{
    Spectrum result(0.0f);

    for (int s = 0; s < lightPath.length; s++)
    {
        for (int t = 1; t < cameraPath.length; t++)
        {
        }
    }
    return result;
}