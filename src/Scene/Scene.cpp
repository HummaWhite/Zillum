#include "../../include/Core/Scene.h"

Scene::Scene(const std::vector<HittablePtr> &hittables, EnvPtr environment, CameraPtr camera) :
    mHittables(hittables), mEnv(environment), mCamera(camera)
{
    for (const auto &i : hittables)
    {
        if (i->type() == HittableType::Light)
            mLights.push_back(std::shared_ptr<Light>(dynamic_cast<Light*>(i.get())));
    }
}

void Scene::setupLightSampleTable()
{
    std::vector<float> lightPdf;
    for (const auto &lt : mLights)
    {
        float pdf = lt->luminance();
        lightPdf.push_back(pdf);
    }
    mLightDistrib = Piecewise1D(lightPdf);
}

std::optional<LightSample> Scene::sampleOneLight(Vec2f u)
{
    if (mLights.size() == 0)
        return std::nullopt;
    bool sampleByPower = mLightSampleStrategy == LightSampleStrategy::ByPower;
    int index = sampleByPower ? mLightDistrib.sample(u) : static_cast<int>(mLights.size() * u.x);

    auto lt = mLights[index];
    float pdf = sampleByPower ? lt->luminance() / mLightDistrib.sum() : 1.0f / mLights.size();
    return LightSample{ lt, pdf };
}

LightEnvSample Scene::sampleLightAndEnv(Vec2f u1, float u2)
{
    auto lightSample = sampleOneLight(u1);
    if (!lightSample)
        return { mEnv, 1.0f };

    float pdfSampleLight = mLightAndEnvStrategy == LightSampleStrategy::ByPower ?
            mLightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    auto [lt, pdfLight] = lightSample.value();

    if (u2 > pdfSampleLight)
        return { mEnv, 1.0f - pdfSampleLight };
    else
        return { lt, pdfLight * pdfSampleLight };
}

LiSample Scene::sampleLiOneLight(const Vec3f &x, const Vec2f &u1, const Vec2f &u2)
{
    if (mLights.size() == 0)
        return InvalidLiSample;
    
    auto lightSample = sampleOneLight(u1);
    auto [lt, pdfSample] = lightSample.value();

    auto liSample = lt->sampleLi(x, u2);
    if (!liSample)
        return InvalidLiSample;

    auto [Wi, weight, dist, pdf] = liSample.value();

    auto lightRay = Ray(x, Wi).offset();
    float testDist = dist - 1e-4f - 1e-6f;

    if (mBvh->testIntersec(lightRay, testDist) || pdf < 1e-8f)
        return InvalidLiSample;

    pdf *= pdfSample;
    return {Wi, weight / pdf, pdf};
}

LiSample Scene::sampleLiEnv(const Vec3f &x, const Vec2f &u1, const Vec2f &u2)
{
    auto [Wi, weight, pdf] = mEnv->sampleLi(u1, u2);
    auto ray = Ray(x, Wi).offset();
    if (quickIntersect(ray, 1e30f))
        return InvalidLiSample;
    return { Wi, weight / pdf, pdf };
}

LiSample Scene::sampleLiLightAndEnv(const Vec3f &x, const std::array<float, 5> &sample)
{
    float pdfSampleLight = 0.0f;

    if (mLights.size() > 0)
    {
        pdfSampleLight = mLightAndEnvStrategy == LightSampleStrategy::ByPower ?
            mLightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    }

    bool sampleLight = sample[0] < pdfSampleLight;
    float pdfSelect = sampleLight ? pdfSampleLight : 1.0f - pdfSampleLight;

    Vec2f u1(sample[1], sample[2]);
    Vec2f u2(sample[3], sample[4]);

    auto [Wi, coef, pdf] = sampleLight ? sampleLiOneLight(x, u1, u2) : sampleLiEnv(x, u1, u2);
    return {Wi, coef / pdfSelect, pdf * pdfSelect};
}

LeSample Scene::sampleLeOneLight(const std::array<float, 6> &sample)
{
    auto lightSample = sampleOneLight({ sample[0], sample[1] });
    auto [light, pdfLight] = lightSample.value();
    auto [ray, Le, pdfPos, pdfDir] = light->sampleLe(*reinterpret_cast<const std::array<float, 4>*>(&sample[2]));
    Vec3f Nl = light->normalGeom(ray.ori);
    return { ray, Le * Math::satDot(Nl, ray.dir), pdfLight * pdfPos * pdfDir };
}

LeSample Scene::sampleLeEnv(const std::array<float, 6> &sample)
{
    auto [ray, Le, pdfPos, pdfDir] = mEnv->sampleLe(mBoundRadius, sample);
    ray.ori += mBound.centroid();
    return { ray, Le, pdfPos * pdfDir };
}

LeSample Scene::sampleLeLightAndEnv(const std::array<float, 7> &sample)
{
    float pdfSelectLight = 0.0f;
    if (mLights.size() > 0)
    {
        pdfSelectLight = mLightAndEnvStrategy == LightSampleStrategy::ByPower ?
            mLightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    }
    bool selectLight = sample[0] < pdfSelectLight;
    float pdfSelect = selectLight ? pdfSelectLight : 1.0f - pdfSelectLight;

    return selectLight ?
        sampleLeOneLight(*reinterpret_cast<const std::array<float, 6>*>(&sample[1])) :
        sampleLeEnv(*reinterpret_cast<const std::array<float, 6>*>(&sample[1]));
}

float Scene::pdfSampleLight(Light *lt)
{
    if (mLights.size() == 0)
        return 0.0f;

    float fstPdf = mLightSampleStrategy == LightSampleStrategy::ByPower ?
        lt->luminance() / mLightDistrib.sum() :
        1.0f / mLights.size();

    float sndPdf = mLightAndEnvStrategy == LightSampleStrategy::ByPower ?
        mLightDistrib.sum() / powerlightAndEnv() :
        0.5f;

    return fstPdf * sndPdf;
}

float Scene::pdfSampleEnv()
{
    return mLightAndEnvStrategy == LightSampleStrategy::ByPower ?
        mEnv->power() / (mLightDistrib.sum() + mEnv->power()) :
        0.5f;
}

IiSample Scene::sampleIiCamera(Vec3f x, Vec2f u)
{
    auto sample = mCamera->sampleIi(x, u);
    if (!sample)
        return InvalidIiSample;
    auto [Wi, imp, dist, uv, pdf] = sample.value();

    auto camRay = Ray(x, Wi).offset();
    float testDist = dist - 1e-4f - 1e-6f;

    if (mBvh->testIntersec(camRay, testDist) || pdf < 1e-8f)
        return InvalidIiSample;
    return { Wi, imp / pdf, pdf };
}

void Scene::buildScene()
{
    mBvh = std::make_shared<BVH>(mHittables);
    setupLightSampleTable();
    mBound = mBvh->box();
    mBoundRadius = glm::distance(mBound.pMin, mBound.pMax) * 0.5f;
}

void Scene::addLight(LightPtr light)
{
    mLights.push_back(light);
    mHittables.push_back(light);
}

void Scene::addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material)
{
    auto [vertices, texcoords, normals] = ObjReader::readFile(path);
    int faceCount = vertices.size() / 3;
    for (int i = 0; i < faceCount; i++)
    {
        Vec3f v[] = {vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]};
        Vec3f n[] = {normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]};
        Vec2f t[3];

        auto tr = std::make_shared<MeshTriangle>(v, t, n);
        tr->setTransform(transform);
        mHittables.push_back(std::make_shared<Object>(tr, material));
    }
}

void Scene::addLightMesh(const char *path, TransformPtr transform, const Spectrum &power)
{
    auto [vertices, texcoords, normals] = ObjReader::readFile(path);
    int faceCount = vertices.size() / 3;
    for (int i = 0; i < faceCount; i++)
    {
        Vec3f v[] = {vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]};
        Vec3f n[] = {normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]};
        Vec2f t[3];

        auto tr = std::make_shared<Light>(std::make_shared<MeshTriangle>(v, t, n), power, false);

        tr->setTransform(transform);
        mHittables.push_back(tr);
        mLights.push_back(tr);
    }
}

bool Scene::visible(Vec3f x, Vec3f y)
{
    float dist = glm::distance(x, y) - 2e-5f;
    Vec3f Wi = glm::normalize(y - x);
    Ray ray(x + Wi * 1e-5f, Wi);
    return !mBvh->testIntersec(ray, dist);
}

float Scene::v(Vec3f x, Vec3f y)
{
    return visible(x, y) ? 1.0f : 0.0f;
}

float Scene::g(Vec3f x, Vec3f y, Vec3f Nx, Vec3f Ny)
{
    if (!visible(x, y))
        return 0.0f;
    Vec3f W = y - x;
    float r = glm::length(W);
    W /= r;
    return Math::satDot(Nx, W) * Math::satDot(Ny, -W) / (r * r);
}