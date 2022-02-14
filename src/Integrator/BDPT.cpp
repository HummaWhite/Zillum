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

    Vertex(const Vec3f &P, const Vec3f &N, Camera *camera) : P(P), N(N), camera(camera), type(VertexType::Camera) {}

    Vertex(const Vec3f &P, const Vec3f &N, const Vec3f &Wo, Material *bsdf) : P(P), N(N), W(Wo), bsdf(bsdf), type(VertexType::Surface) {}

    Vertex(const Vec3f &Wi, Environment *env) : W(Wi), envLight(env), type(VertexType::EnvLight) {}

    Vertex(const Vec3f &P, const Vec3f &N, Light *light) : P(P), N(N), areaLight(light), type(VertexType::AreaLight) {}

    bool isEndPoint() const
    {
        return type != VertexType::Surface;
    }

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

struct TransportPath
{
    void addVertex(const Vertex &v)
    {
        vertices[length++] = v;
    }

    const Vertex &at(int index) const
    {
        return vertices[index];
    }

    Vertex *operator()(int index)
    {
        if (index < 0 || index >= TracingDepthLimit)
            return nullptr;
        return vertices + index;
    }

    Vertex &operator[](int index)
    {
        return vertices[index];
    }

    Vertex vertices[TracingDepthLimit];
    int length = 0;
};

float g(const Vertex &a, const Vertex &b, ScenePtr scene)
{
    return scene->g(a.P, b.P, a.N, b.N);
}

float convertPdfToArea(const Vec3f &fr, const Vec3f &to, const Vec3f &N, float pdfSolidAngle)
{
    float dist2 = Math::distSquare(fr, to);
    return pdfSolidAngle * Math::absDot(N, glm::normalize(fr - to)) / dist2;
}

float convertPdf(const Vertex &fr, const Vertex &to, float pdfSolidAngle)
{
    return (to.type == VertexType::EnvLight) ? pdfSolidAngle : convertPdfToArea(fr.P, to.P, to.N, pdfSolidAngle);
}

Spectrum bsdf(const Vertex &fr, const Vertex &to, TransportMode mode)
{
    Vec3f Wi = glm::normalize(to.P - fr.P);
    return fr.bsdf->bsdf(fr.N, fr.W, Wi, mode);
}

float threePointPdf(const Vertex *prev, const Vertex &fr, const Vertex &to)
{
    Vec3f Wi = glm::normalize(to.P - fr.P);
    if (fr.type == VertexType::Surface)
    {
        Vec3f Wo = glm::normalize(prev->P - fr.P);
        return convertPdf(fr, to, fr.bsdf->pdf(fr.N, Wo, Wi));
    }
    else if (fr.type == VertexType::Camera)
        return convertPdf(fr, to, fr.camera->pdfIe(Ray(fr.P, Wi)).pdfDir);
    else if (fr.type == VertexType::AreaLight)
        return convertPdf(fr, to, fr.areaLight->pdfLe(Ray(fr.P, Wi)).pdfDir);
    // TODO: handle environment light
    return 0;
}

void generateLightPath(const BDPTIntegParam &param, ScenePtr scene, SamplerPtr sampler, TransportPath &path)
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
    // TODO: handle environment light
    if (lightSource.index() != 0)
        return;
    auto light = std::get<0>(lightSource);

    auto leSamp = light->sampleLe(sampler->get<4>());
    Vec3f Nl = light->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;

    Vertex vertex(leSamp.ray.ori, Nl, light.get());
    vertex.throughput = leSamp.Le;
    vertex.pdfCamward = pdfSource * leSamp.pdfPos;
    vertex.isDelta = false; // light->isDelta();
    path.addVertex(vertex);

    Ray ray = leSamp.ray.offset();
    Vec3f throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);
    float pdfSolidAngle = leSamp.pdfDir;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object *>(hit.get());

        Vec3f P = ray.get(hitDist);
        auto surf = obj->surfaceInfo(P);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        vertex = Vertex(P, surf.NShad, Wo, surf.material.get());
        vertex.throughput = throughput;
        vertex.pdfCamward = convertPdf(path[bounce - 1], vertex, pdfSolidAngle);
        vertex.isDelta = deltaBsdf;
        path.addVertex(vertex);

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;

        if (bounce >= param.rrCameraStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.russianRoulette)
            break;

        throughput *= bsdf * NoWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;

        ray = Ray(P, Wi).offset();
        Wo = -Wi;
    }
}

void generateCameraPath(const BDPTIntegParam &param, ScenePtr scene, Ray ray, SamplerPtr sampler, TransportPath &path)
{
    auto camera = scene->mCamera;
    auto [pdfCamPos, pdfCamDir] = camera->pdfIe(ray);
    Vec3f Nc = camera->f();

    Vertex vertex(ray.ori, Nc, camera.get());
    vertex.throughput = Spectrum(1.0f);
    vertex.pdfLitward = pdfCamPos;
    vertex.isDelta = false; // camera->isDelta();
    path.addVertex(vertex);

    Spectrum throughput = camera->Ie(ray) * Math::absDot(Nc, ray.dir) / (pdfCamPos * pdfCamDir);
    Vec3f Wo = -ray.dir;
    float pdfSolidAngle = pdfCamDir;
    float etaScale = 1.0f;

    for (int bounce = 1; bounce < TracingDepthLimit; bounce++)
    {
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
        {
            // TODO: create environment light vertex
            break;
        }
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object *>(hit.get());

        Vec3f P = ray.get(hitDist);
        auto surf = obj->surfaceInfo(P);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        vertex = Vertex(P, surf.NShad, Wo, surf.material.get());
        vertex.throughput = throughput;
        vertex.pdfLitward = convertPdf(path[bounce - 1], vertex, pdfSolidAngle);
        vertex.isDelta = deltaBsdf;
        path.addVertex(vertex);

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Radiance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        if (type.isTransmission())
            etaScale *= Math::square(eta);

        if (bounce >= param.rrCameraStartDepth && param.russianRoulette)
        {
            float continueProb = glm::min<float>(Math::maxComponent(bsdf / bsdfPdf) * etaScale, 0.95f);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.russianRoulette)
            break;

        throughput *= bsdf * NoWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;
        ray = Ray(P, Wi).offset();
        Wo = -Wi;
    }
}

void calculateReversedPdf(TransportPath &lightPath, TransportPath &cameraPath)
{
    for (int i = 1; i < lightPath.length - 1; i++)
    {
        auto &fr = lightPath[i];
        auto &to = lightPath[i - 1];
        Vec3f Wi = glm::normalize(lightPath[i + 1].P - fr.P);
        float pdfSolidAngle = fr.bsdf->pdf(fr.N, Wi, fr.W, TransportMode::Radiance);
        // TODO: handle environment light here
        to.pdfLitward = convertPdf(fr, to, pdfSolidAngle);
    }
    for (int i = 1; i < cameraPath.length - 1; i++)
    {
        auto &fr = cameraPath[i];
        auto &to = cameraPath[i - 1];
        Vec3f Wi = glm::normalize(cameraPath[i + 1].P - fr.P);
        float pdfSolidAngle = fr.bsdf->pdf(fr.N, Wi, fr.W, TransportMode::Importance);
        to.pdfCamward = convertPdf(fr, to, pdfSolidAngle);
    }
}

Spectrum connectPaths(const TransportPath &lightPath, const TransportPath &cameraPath, int s, int t,
                      ScenePtr scene, SamplerPtr sampler, bool resampleDirect, std::optional<Vec2f> &uvRaster)
{
    if (s == 0)
    {
        // naive PT
        // since the case of environment light is currently omitted
        // no need to implement now
        return Spectrum(0.0f);
    }
    auto &vs = lightPath.at(s - 1);
    auto &vt = cameraPath.at(t - 1);

    Spectrum result;
    Vertex sampled;
    if (s == 1)
    {
        if (resampleDirect)
        {
            auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
            if (lightSource.index() == 1)
                // TODO: environment light
                return Spectrum(0.0f);
            auto light = std::get<0>(lightSource);

            auto LiSample = light->sampleLi(vt.P, sampler->get2());
            if (!LiSample)
                return Spectrum(0.0f);
            auto [Wi, Li, dist, pdfLi] = LiSample.value();
            if (Math::isBlack(Li) || pdfLi == 0)
                return Spectrum(0.0f);
            
            Vec3f Pl = Ray(vt.P, Wi).get(dist);
            if (!scene->visible(vt.P, Pl))
                return Spectrum(0.0f);

            sampled = Vertex(Pl, light->normalGeom(Pl), light.get());
            sampled.throughput = Li / (pdfLi * pdfSource);
            sampled.pdfCamward = pdfSource;
            sampled.isDelta = false; // light->isDelta();

            result = sampled.throughput * bsdf(vt, sampled, TransportMode::Radiance) * vt.throughput *
                Math::absDot(vt.N, Wi);
        }
        else
        {
            if (vs.isDelta)
                return Spectrum(0.0f);
            if (!scene->visible(vs.P, vt.P))
                return Spectrum(0.0f);
            Vec3f Wi = glm::normalize(vs.P - vt.P);
            Spectrum Le = vs.areaLight->Le({ vs.P, -Wi });
            float pdf = scene->pdfSampleLight(vs.areaLight);
            sampled = vs;
            sampled.throughput = Le / pdf;
            sampled.pdfCamward = pdf;

            result = sampled.throughput * bsdf(vt, sampled, TransportMode::Radiance) * vt.throughput *
                Math::absDot(vt.N, Wi);
        }
    }
    else if (t == 1)
    {
        if (resampleDirect)
        {
            auto IiSample = vt.camera->sampleIi(vs.P, sampler->get2());
            if (!IiSample)
                return Spectrum(0.0f);
            auto [Wi, Ii, dist, uv, pdf] = IiSample.value();
            Vec3f Pc = Ray(vs.P, Wi).get(dist);
            uvRaster = uv;

            if (!scene->visible(vs.P, Pc))
                return Spectrum(0.0f);

            sampled = Vertex(Pc, vt.camera->f(), vt.camera);
            sampled.throughput = Ii / pdf;
            sampled.pdfLitward = pdf;
            sampled.isDelta = false; // camera->isDelta();

            result = sampled.throughput * bsdf(vs, sampled, TransportMode::Importance) * vs.throughput *
                Math::absDot(vs.N, Wi);
        }
        else
        {
            if (!scene->visible(vt.P, vs.P))
                return Spectrum(0.0f);
            Vec3f Wi = glm::normalize(vt.P - vs.P);
            Ray ray(vt.P, -Wi);
            Spectrum Ie = vt.camera->Ie(ray);
            uvRaster = vt.camera->rasterPos(ray);
            sampled = vt;
            sampled.throughput = Ie;
            sampled.pdfLitward = 1.0f;

            result = sampled.throughput * bsdf(vs, sampled, TransportMode::Importance) * vs.throughput *
                Math::absDot(vs.N, Wi);
        }
    }

    if (vs.isDelta || vt.isDelta)
        return Spectrum(0.0f);
    Spectrum L = vs.throughput * bsdf(vs, vt, TransportMode::Importance) *
        bsdf(vt, vs, TransportMode::Radiance) * vt.throughput;
    if (!Math::isBlack(L))
        L *= g(vs, vt, scene);
    return L;
}

Spectrum eval(const TransportPath &lightPath, const TransportPath &cameraPath, ScenePtr scene, SamplerPtr sampler,
    bool resampleDirect)
{
    Spectrum result(0.0f);
    for (int s = 0; s < lightPath.length; s++)
    {
        for (int t = 1; t < cameraPath.length; t++)
        {
            std::optional<Vec2f> uvRaster;
            Spectrum est = connectPaths(lightPath, cameraPath, s, t, scene, sampler, resampleDirect, uvRaster);
        }
    }
    return result;
}

Spectrum BDPTIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    TransportPath lightPath, cameraPath;
    generateLightPath(mParam, mScene, mLightSampler, lightPath);
    generateCameraPath(mParam, mScene, ray, sampler, cameraPath);
    calculateReversedPdf(lightPath, cameraPath);
    return eval(lightPath, cameraPath, mScene, sampler, mParam.resampleDirect);
}