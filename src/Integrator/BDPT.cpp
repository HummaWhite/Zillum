#include "../../include/Core/Integrator.h"

enum class VertexType
{
    EnvLight,
    AreaLight,
    Surface,
    Camera
};

struct Vertex
{
    Vertex() = default;

    Vertex(const Vec3f &P, const Vec3f &N, Camera *camera) :
        P(P), N(N), camera(camera), type(VertexType::Camera) {}

    Vertex(const Vec3f &P, const Vec3f &N, Material *bsdf) :
        P(P), N(N), bsdf(bsdf), type(VertexType::Surface) {}

    Vertex(const Vec3f &Wi, Environment *env) :
        W(Wi), envLight(env), type(VertexType::EnvLight) {}

    Vertex(const Vec3f &P, const Vec3f &N, Light *light) :
        P(P), N(N), areaLight(light), type(VertexType::AreaLight) {}

    Vec3f P;
    Vec3f N;
    Vec3f W;

    float pdfLitward;
    float pdfCamward;
    Spectrum throughput;
    bool isDelta;

    union
    {
        Environment *envLight;
        Light *areaLight;
        Material *bsdf;
        Camera *camera;
    };

    VertexType type;
};

struct VertexPath
{
	void addVertex(const Vertex &v)
	{
		vertices[length++] = v;
	}
	Vertex vertices[TracingDepthLimit];
	int length = 0;
};

float g(const Vec3f &a, const Vec3f &b, const Vec3f &Na, const Vec3f &Nb)
{
    float dist2 = Math::distSquare(a, b);
    Vec3f W = glm::normalize(b - a);
    return Math::absDot(Na, W) * Math::absDot(Nb, W) / dist2;
}

float convertPdfToArea(const Vec3f &fr, const Vec3f &to, const Vec3f &N, float pdfSolidAngle)
{
    float dist2 = Math::distSquare(fr, to);
    return pdfSolidAngle * Math::absDot(N, glm::normalize(fr - to)) / dist2;
}

void generateLightPath(const BDPTIntegParam &param, ScenePtr scene, SamplerPtr sampler, VertexPath &path)
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
    if (lightSource.index() != 0)
        return;
    auto light = std::get<0>(lightSource);

    auto leSamp = light->sampleLe(sampler->get<4>());
    Vec3f Nl = light->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;

    Ray ray = leSamp.ray.offset();
    Vec3f throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);
    float pdfSolidAngle = leSamp.pdfDir;

    Vertex lastVertex(leSamp.ray.ori, Nl, light.get());
    lastVertex.pdfCamward = pdfSource * leSamp.pdfPos;
    lastVertex.isDelta = false; // light->isDelta();
    path.addVertex(lastVertex);

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f P = ray.get(hitDist);
        auto surf = obj->surfaceInfo(P);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();
        
        float pdfArea = convertPdfToArea(ray.ori, P, surf.NShad, pdfSolidAngle);
        lastVertex = Vertex(P, surf.NShad, surf.material.get());
        lastVertex.pdfCamward = pdfArea;
        lastVertex.isDelta = deltaBsdf;
        path.addVertex(lastVertex);

        if (bounce >= param.rrCameraStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(throughput));
            float rr = sampler->get1();
            if (rr > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.russianRoulette)
            break;

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;
        ray = Ray(P, Wi).offset();
        Wo = -Wi;
    }
}

void generateCameraPath(const BDPTIntegParam &param, ScenePtr scene, Ray ray, SamplerPtr sampler, VertexPath &path)
{
    auto camera = scene->mCamera;
    auto [pdfCamPos, pdfSolidAngle] = camera->pdfIe(ray);
    Vec3f Nc = camera->f();

    Vertex lastVertex(ray.ori, Nc, camera.get());
    lastVertex.pdfLitward = pdfCamPos;
    lastVertex.isDelta = false; // camera->isDelta();

    Spectrum throughput = camera->Ie(ray) * Math::absDot(Nc, ray.dir) / (pdfCamPos * pdfSolidAngle);
    Vec3f Wo = -ray.dir;
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object*>(hit.get());

        Vec3f P = ray.get(hitDist);
        auto surf = obj->surfaceInfo(P);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        float pdfArea = convertPdfToArea(ray.ori, P, surf.NShad, pdfSolidAngle);
        lastVertex = Vertex(P, surf.NShad, surf.material.get());
        lastVertex.pdfLitward = pdfArea;
        lastVertex.isDelta = deltaBsdf;
        path.addVertex(lastVertex);

        if (bounce >= param.rrCameraStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(0.9f, Math::maxComponent(throughput) * etaScale);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.russianRoulette)
            break;

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Radiance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        if (type.isTransmission())
            etaScale *= Math::square(eta);

        throughput *= bsdf * NoWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;
        ray = Ray(P, Wi).offset();
        Wo = -Wi;
    }
}

Spectrum connect(const VertexPath &lightPath, const VertexPath &cameraPath)
{
    return Spectrum(0.0f);
}

Spectrum eval(const VertexPath &lightPath, const VertexPath &cameraPath, ScenePtr scene)
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

Spectrum BDPTIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    VertexPath lightPath, cameraPath;
    generateLightPath(mParam, mScene, mLightSampler, lightPath);
    generateCameraPath(mParam, mScene, ray, sampler, cameraPath);
    return eval(lightPath, cameraPath, mScene);
}