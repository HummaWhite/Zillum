#include "../../include/Core/Integrator.h"

struct PathVertex
{
	PathVertex() = default;

    PathVertex(const Vec3f &P, const Vec3f &N, const Vec3f &throughput) :
        P(P), N(N), material(nullptr), throughput(throughput) {}

	PathVertex(const Vec3f &P, const Vec3f &N, const Vec3f &Wo, MaterialPtr material, const Vec3f &throughput) :
		P(P), N(N), Wo(Wo), material(material), throughput(throughput) {}

	bool isEndPoint() const { return material == nullptr; }

	Vec3f P;
	Vec3f N;
	Vec3f Wo;
	MaterialPtr material;
	Spectrum throughput;
};

Spectrum AdjointPathIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto [dist, obj] = mScene->closestHit(ray);

    if (obj == nullptr)
        return mScene->mEnv->radiance(ray.dir);

    if (obj->type() == HittableType::Light)
    {
        auto lt = dynamic_cast<Light *>(obj.get());
        auto y = ray.get(dist);
        return lt->Le({y, -ray.dir});
    }
    else if (obj->type() == HittableType::Object)
    {
        Vec3f p = ray.get(dist);
        auto tmp = obj.get();
        auto ob = dynamic_cast<Object *>(obj.get());
        auto surf = ob->surfaceInfo(p);
        ray.ori = p;

        auto vLight = traceLight(sampler);
        auto vCamera = traceCamera(ray, surf, sampler);

        if (!vLight || !vCamera)
            return Spectrum(0.0f);
        return connect(vLight.value(), vCamera.value());
    }
    Error::impossiblePath();
    return Spectrum(0.0f);
}

std::optional<PathVertex> AdjointPathIntegrator::traceLight(SamplerPtr sampler)
{
    auto [lightSource, pdfSource] = mScene->sampleLightAndEnv(sampler->get2(), sampler->get1());
    if (lightSource.index() != 0)
        return std::nullopt;
    auto lt = std::get<0>(lightSource);

    auto leSamp = lt->sampleLe(sampler->get<4>());
    Vec3f Nl = lt->normalGeom(leSamp.ray.ori);
    Vec3f Wo = -leSamp.ray.dir;
    Ray ray = leSamp.ray.offset();
    Spectrum throughput = leSamp.Le * Math::absDot(Nl, -Wo) / (pdfSource * leSamp.pdfPos * leSamp.pdfDir);

    for (int bounce = 1;; bounce++)
    {
        auto [distObj, hit] = mScene->closestHit(ray);
        if (!hit)
            break;
        if (hit->type() != HittableType::Object)
            break;
        auto obj = dynamic_cast<Object *>(hit.get());

        Vec3f pShading = ray.get(distObj);
        auto surf = obj->surfaceInfo(pShading);

        if (glm::dot(surf.NShad, Wo) < 0)
        {
            auto bxdf = surf.material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                surf.NShad = -surf.NShad;
        }
        bool deltaBsdf = surf.material->bxdf().isDelta();

        auto sample = surf.material->sample(surf.NShad, Wo, sampler->get1(), sampler->get2(), TransportMode::Importance);
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        PathVertex lightVertex(pShading, surf.NShad, Wo, surf.material, throughput);
        if (!russianRouletteLight(glm::min<float>(1.0f, Math::maxComponent(bsdf / bsdfPdf)), bounce, sampler, throughput))
            return lightVertex;

        float NoWi = deltaBsdf ? 1.0f : Math::satDot(surf.NShad, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;
        ray = Ray(pShading, Wi).offset();
        Wo = -Wi;
    }
    return std::nullopt;
}

std::optional<PathVertex> AdjointPathIntegrator::traceCamera(Ray ray, SurfaceInfo surf, SamplerPtr sampler)
{
    Spectrum throughput(1.0f);
    float etaScale = 1.0f;
    for (int bounce = 1;; bounce++)
    {
        Vec3f P = ray.ori;
        Vec3f Wo = -ray.dir;
        Vec3f N = surf.NShad;
        MaterialPtr material = surf.material;

        if (glm::dot(N, Wo) < 0)
        {
            auto bxdf = material->bxdf();
            if (!bxdf.hasType(BXDF::GlosTrans) && !bxdf.hasType(BXDF::SpecTrans))
                N = -N;
        }
        bool deltaBsdf = material->bxdf().isDelta();

        auto sample = surf.material->sample(N, Wo, sampler->get1(), sampler->get2());
        if (!sample)
            break;
        auto [Wi, bsdfPdf, type, eta, bsdf] = sample.value();

        float NoWi = type.isDelta() ? 1.0f : Math::absDot(N, Wi);
        if (bsdfPdf < 1e-8f || Math::isNan(bsdfPdf) || Math::isInf(bsdfPdf))
            break;
        throughput *= bsdf * NoWi / bsdfPdf;

        auto newRay = Ray(P, Wi).offset();
        auto [dist, obj] = mScene->closestHit(newRay);

        if (obj == nullptr)
            break;
        if (obj->type() == HittableType::Light)
            break;

        if (type.isTransmission())
            etaScale *= Math::square(eta);

        PathVertex cameraVertex(P, N, Wo, material, throughput);
        if (!russianRouletteCamera(glm::min<float>(0.9f, Math::maxComponent(bsdf / bsdfPdf) * etaScale), bounce, sampler, throughput))
            return cameraVertex;

        Vec3f nextP = newRay.get(dist);
        auto ob = dynamic_cast<Object *>(obj.get());
        surf = ob->surfaceInfo(nextP);
        newRay.ori = nextP;
        ray = newRay;
    }
    return std::nullopt;
}

Spectrum AdjointPathIntegrator::connect(const PathVertex &vLight, const PathVertex &vCamera)
{
    if (vLight.material->bxdf().isDelta() || vCamera.material->bxdf().isDelta())
        return Spectrum(0.0f);

    Vec3f camToLight = glm::normalize(vLight.P - vCamera.P);
    float g = mScene->g(vCamera.P, vLight.P, vCamera.N, vLight.N);
    
    if (vLight.isEndPoint() && vCamera.isEndPoint())
    {
    }
    if (vLight.isEndPoint())
    {
    }
    if (vCamera.isEndPoint())
    {
    }
    Spectrum bsdfCamera = vCamera.material->bsdf(vCamera.N, vCamera.Wo, camToLight, TransportMode::Radiance);
    Spectrum bsdfLight = vLight.material->bsdf(vLight.N, vLight.Wo, -camToLight, TransportMode::Importance);
    return vCamera.throughput * bsdfCamera * vLight.throughput * bsdfLight * g;
}

bool AdjointPathIntegrator::russianRouletteLight(float continueProb, int bounce, SamplerPtr sampler, Spectrum &throughput)
{
    if (bounce >= mRRLightStartDepth && mRussianRoulette)
    {
        if (sampler->get1() > continueProb)
            return false;
        throughput /= continueProb;
    }
    if (bounce >= mMaxLightDepth && !mRussianRoulette)
        return false;
    return true;
}

bool AdjointPathIntegrator::russianRouletteCamera(float continueProb, int bounce, SamplerPtr sampler, Spectrum &throughput)
{
    if (bounce >= mRRCameraStartDepth && mRussianRoulette)
    {
        if (sampler->get1() > continueProb)
            return false;
        throughput /= continueProb;
    }
    if (bounce >= mMaxCameraDepth && !mRussianRoulette)
        return false;
    return true;
}