#include "../../include/Core/Integrator.h"
#include "../../include/Utils/TempAssignment.h"

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

    Vertex(const Vec3f &pos, Camera *camera) :
        pos(pos), normCam(camera->f()), camera(camera), type(VertexType::Camera) {}

    Vertex(const Vec3f &pos, const SurfaceInfo &surf, const Vec3f &wo) :
        pos(pos), normShad(surf.ns), normGeom(surf.ng), dir(wo), bsdf(surf.material.get()), type(VertexType::Surface) {}

    Vertex(const Vec3f &wi, Environment *env) : dir(wi), envLight(env), type(VertexType::EnvLight) {}

    Vertex(const Vec3f &pos, Light *light) :
        pos(pos), normLit(light->normalGeom(pos)), areaLight(light), type(VertexType::AreaLight) {}

    Vec3f getNormal() const
    {
        return (type == VertexType::Surface) ? normShad : normGeom;
    }

    Vec3f pos;
    Vec3f dir;
    Vec3f normShad;
    union 
    {
        Vec3f normGeom;
        Vec3f normCam;
        Vec3f normLit;
    };

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

struct Path
{
    static Vertex createCamera(const Vec3f &pos, Camera *camera) { return Vertex(pos, camera); }
    static Vertex createSurface(const Vec3f &pos, const SurfaceInfo &surf, const Vec3f &wo) { return Vertex(pos, surf, wo); }
    static Vertex createEnvLight(const Vec3f &wi, Environment *env) { return Vertex(wi, env); }
    static Vertex createAreaLight(const Vec3f &pos, Light *light) { return Vertex(pos, light); }

    void addVertex(const Vertex &v)
    {
        vertices[length++] = v;
    }

    Vertex* operator () (int index)
    {
        if (index < 0 || index >= length)
            return nullptr;
        return vertices + index;
    }

    Vertex& operator [] (int index)
    {
        return vertices[index];
    }

    const Vertex& operator [] (int index) const
    {
        return vertices[index];
    }

    Vertex vertices[TracingDepthLimit];
    int length = 0;
};

float g(const Vertex &a, const Vertex &b, ScenePtr scene)
{
    return scene->g(a.pos, b.pos, a.getNormal(), b.getNormal());
}

float gNoVisibility(const Vertex &a, const Vertex &b)
{
    Vec3f w = glm::normalize(a.pos - b.pos);
    return Math::satDot(a.getNormal(), -w) * Math::satDot(b.getNormal(), w) / Math::distSquare(a.pos, b.pos);
}

float convertPdfToArea(const Vec3f &fr, const Vec3f &to, const Vec3f &n, float pdfSolidAngle)
{
    float dist2 = Math::distSquare(fr, to);
    return pdfSolidAngle * Math::satDot(n, glm::normalize(fr - to)) / dist2;
}

float convertPdf(const Vertex &fr, const Vertex &to, float pdfSolidAngle)
{
    return (to.type == VertexType::EnvLight) ? pdfSolidAngle : convertPdfToArea(fr.pos, to.pos, to.getNormal(), pdfSolidAngle);
}

Spectrum bsdf(const Vertex &fr, const Vertex &to, TransportMode mode)
{
    Vec3f wi = glm::normalize(to.pos - fr.pos);
    return fr.bsdf->bsdf(fr.getNormal(), fr.dir, wi, mode);
}

// fr must be a source vertex, either radiance or importance
float twoPointPdf(const Vertex &fr, const Vertex &to)
{
    Vec3f wi = glm::normalize(to.pos - fr.pos);
    if (fr.type == VertexType::Camera)
        return convertPdf(fr, to, fr.camera->pdfIe(Ray(fr.pos, wi)).pdfDir);
    else if (fr.type == VertexType::AreaLight)
        return convertPdf(fr, to, fr.areaLight->pdfLe(Ray(fr.pos, wi)).pdfDir);
    // TODO: handle environment light
    return 0;
}

// fr must be a surface vertex
float threePointPdf(const Vertex &prev, const Vertex &fr, const Vertex &to, TransportMode mode)
{
    Vec3f wi = glm::normalize(to.pos - fr.pos);
    Vec3f wo = glm::normalize(prev.pos - fr.pos);
    return convertPdf(fr, to, fr.bsdf->pdf(fr.getNormal(), wo, wi, mode));
}

float pdfLightOrigin(const Vertex &light, const Vertex &ref, ScenePtr scene)
{
    if (light.type == VertexType::EnvLight)
        // TODO: environment light
        return 0.0f;
    Ray ray(light.pos, glm::normalize(light.pos - ref.pos));
    return light.areaLight->pdfLe(ray).pdfPos * scene->pdfSampleLight(light.areaLight);
}

float pdfLight(const Vertex &light, const Vertex &ref, ScenePtr scene)
{
    if (light.type == VertexType::EnvLight)
        // TODO: env light
        return 0.0f;
    return light.areaLight->pdfLi(ref.pos, light.pos) * scene->pdfSampleLight(light.areaLight);
}

void generateLightPath(const BDPTIntegParam &param, ScenePtr scene, SamplerPtr sampler, Path &path)
{
    auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
    // TODO: handle environment light
    if (lightSource.index() != 0)
        return;
    auto light = std::get<0>(lightSource);

    auto leSamp = light->sampleLe(sampler->get<4>());
    Vec3f wo = -leSamp.ray.dir;

    auto vertex = Path::createAreaLight(leSamp.ray.ori, light.get());
    vertex.throughput = leSamp.Le / (pdfSource * leSamp.pdfPos);
    vertex.pdfCamward = pdfSource * leSamp.pdfPos;
    vertex.isDelta = false; // light->isDelta();
    path.addVertex(vertex);

    Ray ray = leSamp.ray.offset();
    float pdfSolidAngle = leSamp.pdfDir;
    Spectrum throughput = path[0].throughput * Math::satDot(light->normalGeom(leSamp.ray.ori), -wo) / pdfSolidAngle;

    for (int bounce = 1; bounce < param.maxConnectDepth; bounce++)
    {
        if (Math::isBlack(throughput))
            break;
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto object = dynamic_cast<Object*>(hit.get());

        Vec3f pos = ray.get(hitDist);
        auto surf = object->surfaceInfo(pos);

        if (glm::dot(surf.ns, wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        vertex = Path::createSurface(pos, surf, wo);
        vertex.throughput = throughput;
        vertex.pdfCamward = convertPdf(path[bounce - 1], vertex, pdfSolidAngle);
        vertex.isDelta = deltaBsdf;
        path.addVertex(vertex);

        auto sample = surf.material->sample(surf.ns, wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float cosWi = deltaBsdf ? 1.0f : Math::satDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        
        if (bounce >= param.rrLightStartDepth && param.rrLightPath)
        {
            float continueProb = glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf));
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxLightDepth && !param.rrLightPath)
            break;
        if (bounce > 1)
            path[bounce - 2].pdfLitward = threePointPdf(path[bounce], path[bounce - 1], path[bounce - 2],
                TransportMode::Radiance);

        throughput *= bsdf * cosWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;
    }
}

void generateCameraPath(const BDPTIntegParam &param, ScenePtr scene, Ray ray, SamplerPtr sampler, Path &path)
{
    auto camera = scene->mCamera;
    auto [pdfCamPos, pdfSolidAngle] = camera->pdfIe(ray);

    auto vertex = Path::createCamera(ray.ori, camera.get());
    vertex.throughput = Spectrum(1.0f);
    vertex.pdfLitward = 1.0f;
    vertex.isDelta = false; // camera->isDelta();
    path.addVertex(vertex);

    Spectrum throughput(1.0f);
    Vec3f wo = -ray.dir;

    for (int bounce = 1; bounce < param.maxConnectDepth; bounce++)
    {
        if (Math::isBlack(throughput))
            break;
        auto [hitDist, hit] = scene->closestHit(ray);
        if (!hit)
        {
            // TODO: create environment light vertex
            break;
        }

        Vec3f pos = ray.get(hitDist);
        if (hit->type() == HittableType::Light)
        {
            auto light = dynamic_cast<Light*>(hit.get());
            vertex = Path::createAreaLight(pos, light);
            vertex.throughput = throughput;
            vertex.pdfLitward = convertPdf(path[bounce - 1], vertex, pdfSolidAngle);
            vertex.isDelta = false;
            path.addVertex(vertex);

            if (bounce > 1)
                path[bounce - 2].pdfCamward = threePointPdf(path[bounce], path[bounce - 1], path[bounce - 2],
                    TransportMode::Importance);
            break;
        }

        auto object = dynamic_cast<Object*>(hit.get());
        auto surf = object->surfaceInfo(pos);
        if (glm::dot(surf.ns, wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.flipNormal();
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        vertex = Path::createSurface(pos, surf, wo);
        vertex.throughput = throughput;
        vertex.pdfLitward = convertPdf(path[bounce - 1], vertex, pdfSolidAngle);
        vertex.isDelta = deltaBsdf;
        path.addVertex(vertex);

        auto sample = surf.material->sample(surf.ns, wo, sampler->get1(), sampler->get2(), TransportMode::Radiance);
        if (!sample)
            break;
        auto [wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float cosWi = type.isDelta() ? 1.0f : Math::satDot(surf.ns, wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;

        if (bounce >= param.rrCameraStartDepth && param.rrCameraPath)
        {
            float continueProb = glm::min<float>(Math::maxComponent(bsdf / bsdfPdf), 0.95f);
            if (sampler->get1() > continueProb)
                break;
            throughput /= continueProb;
        }
        if (bounce >= param.maxCameraDepth && !param.rrCameraPath)
            break;
        if (bounce > 1)
            path[bounce - 2].pdfCamward = threePointPdf(path[bounce], path[bounce - 1], path[bounce - 2],
                TransportMode::Importance);

        throughput *= bsdf * cosWi / bsdfPdf;
        pdfSolidAngle = bsdfPdf;
        ray = Ray(pos, wi).offset();
        wo = -wi;
    }
}

void debugPrintVertex(const Vertex &vertex, int index)
{
    std::cout << "\t[Vertex " << index << "]\n";
    std::cout << "\t\tpos: " << Math::vec3ToString(vertex.pos) << ", norm: " << Math::vec3ToString(vertex.normGeom) << "\n";
    std::cout << std::fixed << std::setprecision(9);
    std::cout << "\t\tp->cam: " << vertex.pdfCamward << ", p->lit: " << vertex.pdfLitward << ", throughput: " << Math::vec3ToString(vertex.throughput) << "\n";
    std::cout << "\t\tdelta: " << vertex.isDelta << "\n";
}

void debugPrintPath(const Path &lightPath, const Path &cameraPath, int s, int t)
{
    std::cout << "[Light path length: " << lightPath.length << "]\n";
    for (int i = 0; i < s; i++)
        debugPrintVertex(lightPath[i], i);
    std::cout << "[Camera path length: " << cameraPath.length << "]\n";
    for (int i = 0; i < t; i++)
        debugPrintVertex(cameraPath[i], i);
    std::cout << "\n";
}

float MISWeight(Path &lightPath, Path &cameraPath, int s, int t, ScenePtr scene)
{
    auto remapPdf = [](float v) -> float
    {
        return v < 1e-6f ? 1.0f : v * v;
    };

    if (s + t == 2)
        return 1.0f;

    auto vs = lightPath(s - 1);
    auto vt = cameraPath(t - 1);
    auto vsPred = lightPath(s - 2);
    auto vtPred = cameraPath(t - 2);

    TempAssignment<bool> tmpDeltaVs;
    TempAssignment<bool> tmpDeltaVt;
    TempAssignment<float> tmpPdfLitwardVs, tmpPdfLitwardVsPred;
    TempAssignment<float> tmpPdfCamwardVt, tmpPdfCamwardVtPred;
    
    if (s == 0)
    {
        if (vt->type == VertexType::AreaLight)
        {
            auto [pdfPos, pdfDir] = vt->areaLight->pdfLe({ vt->pos, glm::normalize(vtPred->pos - vt->pos) });
            tmpPdfCamwardVt = { vt->pdfCamward, pdfPos * scene->pdfSampleLight(vt->areaLight) };
            tmpPdfCamwardVtPred = { vtPred->pdfCamward, convertPdf(*vt, *vtPred, pdfDir) };
        }
        else if (vt->type == VertexType::EnvLight)
        {
            // TODO: env light
            return 0.0f;
        }
        else
            return 0.0f;
    }
    else
    {
        tmpDeltaVs = { vs->isDelta, false };
        tmpDeltaVt = { vt->isDelta, false };
        tmpPdfLitwardVs = { vs->pdfLitward,
            vtPred ? threePointPdf(*vtPred, *vt, *vs, TransportMode::Radiance) : twoPointPdf(*vt, *vs) };
        tmpPdfCamwardVt = { vt->pdfCamward,
            vsPred ? threePointPdf(*vsPred, *vs, *vt, TransportMode::Importance) : twoPointPdf(*vs, *vt) };

        if (vsPred)
            tmpPdfLitwardVsPred = { vsPred->pdfLitward,
                threePointPdf(*vt, *vs, *vsPred, TransportMode::Radiance) };
        if (vtPred)
            tmpPdfCamwardVtPred = { vtPred->pdfCamward,
                threePointPdf(*vs, *vt, *vtPred, TransportMode::Importance) };
    }

    float sum = 0.0f;
    float r = 1.0f;
    for (int i = s - 1; i > 0; i--)
    {
        r *= remapPdf(lightPath[i].pdfLitward) / remapPdf(lightPath[i].pdfCamward);
        if (!lightPath[i].isDelta && !lightPath[i - 1].isDelta)
            sum += r;
    }
    r = 1.0f;
    for (int i = t - 1; i > 0; i--)
    {
        r *= remapPdf(cameraPath[i].pdfCamward) / remapPdf(cameraPath[i].pdfLitward);
        if (!cameraPath[i].isDelta && !cameraPath[i - 1].isDelta)
            sum += r;
    }
    return 1.0f / (1.0f + sum);
}

Spectrum connectPaths(Path &lightPath, Path &cameraPath, int s, int t,
    ScenePtr scene, SamplerPtr sampler, bool resampleEndPoint, std::optional<Vec2f> &uvRaster)
{
    Spectrum result(0.0f);
    Vertex endPoint;
    TempAssignment<Vertex> tmpEndPoint;

    if (s == 0)
    {
        const auto &vt = cameraPath[t - 1];
        if (vt.type == VertexType::AreaLight)
        {
            const auto &vtPred = cameraPath[t - 2];
            Ray emiRay(vt.pos, glm::normalize(vtPred.pos - vt.pos));
            result = vt.areaLight->Le(emiRay) * vt.throughput;
        }
        else if (vt.type == VertexType::EnvLight)
        {
            // TODO: env light
        }
        else
            return Spectrum(0.0f);
    }
    else if (s == 1)
    {
        const auto &vs = lightPath[0];
        const auto &vt = cameraPath[t - 1];
        if (vt.type != VertexType::Surface)
            return Spectrum(0.0f);
        if (Math::isBlack(vt.throughput))
            return Spectrum(0.0f);
        if (resampleEndPoint)
        {
            if (vt.isDelta)
                return Spectrum(0.0f);
            auto [lightSource, pdfSource] = scene->sampleLightAndEnv(sampler->get2(), sampler->get1());
            if (lightSource.index() == 1)
                // TODO: environment light
                return Spectrum(0.0f);
            auto light = std::get<0>(lightSource);

            auto LiSample = light->sampleLi(vt.pos, sampler->get2());
            if (!LiSample)
                return Spectrum(0.0f);
            auto [wi, Li, dist, pdfLi] = LiSample.value();
            if (pdfLi == 0)
                return Spectrum(0.0f);
            
            Vec3f pLit = Ray(vt.pos, wi).get(dist);
            if (!scene->visible(vt.pos, pLit))
                return Spectrum(0.0f);

            endPoint = Path::createAreaLight(pLit, light.get());
            endPoint.throughput = Li / (pdfLi * pdfSource);
            endPoint.pdfCamward = light->pdfLe({ pLit, -wi }).pdfPos * pdfSource;
            endPoint.isDelta = false; // light->isDelta();
            
            result = endPoint.throughput * bsdf(vt, endPoint, TransportMode::Radiance) * vt.throughput *
                Math::satDot(vt.normShad, wi);
        }
        else
        {
            if (vs.isDelta || vt.isDelta)
                return Spectrum(0.0f);
            if (!scene->visible(vs.pos, vt.pos))
                return Spectrum(0.0f);
            Vec3f wi = glm::normalize(vs.pos - vt.pos);

            Spectrum Le = vs.areaLight->Le({ vs.pos, -wi });
            endPoint = vs;
            endPoint.throughput = Le / vs.pdfCamward;

            result = endPoint.throughput * bsdf(vt, endPoint, TransportMode::Radiance) * vt.throughput *
                gNoVisibility(vt, endPoint);
        }
        tmpEndPoint = { lightPath[0], endPoint };
    }
    else if (t == 1)
    {
        const auto &vs = lightPath[s - 1];
        const auto &vt = cameraPath[0];
        if (Math::isBlack(vs.throughput))
            return Spectrum(0.0f);
        if (resampleEndPoint)
        {
            if (vs.isDelta)
                return Spectrum(0.0f);
            auto IiSample = vt.camera->sampleIi(vs.pos, sampler->get2());
            if (!IiSample)
                return Spectrum(0.0f);
            auto [wi, Ii, dist, uv, pdf] = IiSample.value();
            Vec3f pCam = Ray(vs.pos, wi).get(dist);
            uvRaster = uv;

            if (!scene->visible(vs.pos, pCam))
                return Spectrum(0.0f);
            endPoint = Path::createCamera(pCam, vt.camera);
            endPoint.throughput = Ii / pdf;
            endPoint.pdfLitward = vt.camera->pdfIe({ pCam, -wi }).pdfPos;
            endPoint.isDelta = false; // camera->isDelta();
            
            result = endPoint.throughput * bsdf(vs, endPoint, TransportMode::Importance) * vs.throughput *
                Math::satDot(vs.normShad, wi);
        }
        else
        {
            if (vs.isDelta || vt.isDelta)
                return Spectrum(0.0f);
            if (!scene->visible(vt.pos, vs.pos))
                return Spectrum(0.0f);
            Vec3f wi = glm::normalize(vt.pos - vs.pos);
            Ray ray(vt.pos, -wi);
            uvRaster = vt.camera->rasterPos(ray);
            
            Spectrum Ie = vt.camera->Ie(ray);
            endPoint = vt;
            endPoint.throughput = Ie / vt.pdfLitward;

            result = endPoint.throughput * bsdf(vs, endPoint, TransportMode::Radiance) * vs.throughput *
                gNoVisibility(vs, endPoint);
        }
        tmpEndPoint = { cameraPath[0], endPoint };
    }
    else
    {
        const auto &vs = lightPath[s - 1];
        const auto &vt = cameraPath[t - 1];
        if (Math::isBlack(vs.throughput) || Math::isBlack(vt.throughput))
            return Spectrum(0.0f);
        if (vt.type != VertexType::Surface || vs.isDelta || vt.isDelta)
            return Spectrum(0.0f);
        if (!scene->visible(vt.pos, vs.pos))
            return Spectrum(0.0f);
        result = vs.throughput * bsdf(vs, vt, TransportMode::Importance) * gNoVisibility(vs, vt) *
            bsdf(vt, vs, TransportMode::Radiance) * vt.throughput;
    }
    REPORT_RETURN_IF(Math::hasNan(result), Spectrum(0.0f), "BDPT nan subpath connection")
    if (Math::isBlack(result))
        return Spectrum(0.0f);

    float weight = MISWeight(lightPath, cameraPath, s, t, scene);
    REPORT_IF(weight > 1.0f, "BDPT weight > 1 for (" << s << ", " << t << ")")
    REPORT_RETURN_IF(Math::isNan(weight), Spectrum(0.0f), "BDPT nan MIS weight")
    return result * weight;
    //return Spectrum(weight);
    //return RGB24::threeFourthWheel(weight);
}

Spectrum BDPTIntegrator::eval(Path &lightPath, Path &cameraPath, SamplerPtr sampler)
{
    Spectrum result(0.0f);
    for (int s = 0; s <= lightPath.length && s <= mParam.maxConnectDepth; s++)
    {
        for (int t = 1; (t <= cameraPath.length) && (s + t <= mParam.maxConnectDepth); t++)
        {
            if (s == 1 && t == 1)
                continue;
                
            if (mParam.debug && (s != mParam.debugStrategy.x || t != mParam.debugStrategy.y))
                continue;

            std::optional<Vec2f> uvRaster;
            Spectrum est = connectPaths(lightPath, cameraPath, s, t, mScene, sampler, mParam.resampleEndPoint, uvRaster);

            if (Math::isBlack(est))
                continue;
            if (uvRaster)
                addToFilmLocked(*uvRaster, est);
            else
                result += est;
        }
    }
    return result;
}

Spectrum BDPTIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    Path lightPath, cameraPath;
    generateLightPath(mParam, mScene, mLightSampler, lightPath);
    generateCameraPath(mParam, mScene, ray, sampler, cameraPath);
    return eval(lightPath, cameraPath, sampler);
}

void BDPTIntegrator::scaleResult()
{
    mResultScale = 1.0f / mCurspp;
}

void BDPTIntegrator2::renderOnePass()
{
    if (mMaxSpp && mParam.spp >= mMaxSpp)
    {
        mFinished = true;
        return;
    }
    auto &film = mScene->mCamera->film();
    int pathsOnePass = mPathsOnePass ? mPathsOnePass : film.width * film.height / MaxThreads;
    std::thread *threads = new std::thread[MaxThreads];
    for (int i = 0; i < MaxThreads; i++)
    {
        auto cameraSampler = mSampler->copy();
        auto lightSampler = mLightSampler->copy();
        cameraSampler->nextSamples(pathsOnePass * i);
        lightSampler->nextSamples(pathsOnePass * i);
        threads[i] = std::thread(trace, this, pathsOnePass, lightSampler, cameraSampler);
    }
    for (int i = 0; i < MaxThreads; i++)
        threads[i].join();
    delete[] threads;

    mSampler->nextSamples(pathsOnePass * MaxThreads);
    mLightSampler->nextSamples(pathsOnePass * MaxThreads);
    mParam.spp += static_cast<float>(pathsOnePass) * MaxThreads / (film.width * film.height);
    mResultScale = 1.0f / mParam.spp;
    std::cout << "\r[BDPTIntegrator2 spp: " << std::fixed << std::setprecision(3) << mParam.spp << "]";
}

void BDPTIntegrator2::reset()
{
    mScene->mCamera->film().fill(Spectrum(0.0f));
    mParam.spp = 0;
}

void BDPTIntegrator2::trace(int paths, SamplerPtr lightSampler, SamplerPtr cameraSampler)
{
    for (int i = 0; i < paths; i++)
    {
        traceOnePath(lightSampler, cameraSampler);
        lightSampler->nextSample();
        cameraSampler->nextSample();
    }
}

void BDPTIntegrator2::traceOnePath(SamplerPtr lightSampler, SamplerPtr cameraSampler)
{
    Path lightPath, cameraPath;
    generateLightPath(mParam, mScene, lightSampler, lightPath);

    Vec2f uv = cameraSampler->get2();
    Ray ray = mScene->mCamera->generateRay(uv * Vec2f(2.0f, -2.0f) + Vec2f(-1.0f, 1.0f), cameraSampler);
    generateCameraPath(mParam, mScene, ray, cameraSampler, cameraPath);

    if (mParam.debug)
    {
        int s = mParam.debugStrategy.x;
        int t = mParam.debugStrategy.y;
        if (s <= lightPath.length && t <= cameraPath.length)
        {
            std::optional<Vec2f> uvRaster;
            Spectrum est = connectPaths(lightPath, cameraPath, s, t, mScene, lightSampler, mParam.resampleEndPoint, uvRaster);
            if (uvRaster)
                addToFilmLocked(*uvRaster, est);
            else
                addToFilmLocked(uv, est);
        }
        return;
    }

    Spectrum result(0.0f);
    
    if (mParam.stochasticConnect)
    {
        for (int depth = 2; depth <= lightPath.length + cameraPath.length; depth++)
        {
            int t = glm::clamp<int>(depth * lightSampler->get1() + 1, 1, depth);
            int s = depth - t;

            if ((s == 1 && t == 1) || s < 0 || s > lightPath.length || t > cameraPath.length)
                continue;

            std::optional<Vec2f> uvRaster;
            Spectrum est = connectPaths(lightPath, cameraPath, s, t, mScene, lightSampler, mParam.resampleEndPoint, uvRaster) *
                static_cast<float>(depth);

            if (Math::isBlack(est))
                continue;
            if (uvRaster)
                addToFilmLocked(*uvRaster, est);
            else
                result += est;
        }
    }
    else
    {
        for (int depth = 2; depth <= mParam.maxConnectDepth; depth++)
        {
            for (int t = 1; t <= cameraPath.length; t++)
            {
                int s = depth - t;
                if ((s == 1 && t == 1) || s < 0 || s > lightPath.length)
                    continue;

                std::optional<Vec2f> uvRaster;
                Spectrum est = connectPaths(lightPath, cameraPath, s, t, mScene, lightSampler, mParam.resampleEndPoint, uvRaster);

                if (Math::isBlack(est))
                    continue;
                if (uvRaster)
                    addToFilmLocked(*uvRaster, est);
                else
                    result += est;
            }
        }
    }
    
    if (!Math::isBlack(result))
        addToFilmLocked(uv, result);
}