#include "Scene.h"

Scene::Scene(const std::vector<HittablePtr> &hittables, EnvPtr environment, CameraPtr camera) :
    hittables(hittables), env(environment), camera(camera)
{
    for (const auto &i : hittables)
    {
        if (i->type() == HittableType::Light)
            lights.push_back(std::shared_ptr<Light>(dynamic_cast<Light*>(i.get())));
    }
}

void Scene::setupLightSampleTable()
{
    std::vector<float> lightPdf;
    for (const auto &lt : lights)
    {
        float pdf = lt->getRgbPower();
        lightPdf.push_back(pdf);
    }
    lightDistrib = Piecewise1D(lightPdf);
}

std::optional<LightSample> Scene::sampleOneLight(glm::vec2 u)
{
    if (lights.size() == 0)
        return std::nullopt;
    bool sampleByPower = lightSampleStrategy == LightSampleStrategy::ByPower;
    int index = sampleByPower ? lightDistrib.sample(u) : static_cast<int>(lights.size() * u.x);

    auto lt = lights[index];
    float pdf = sampleByPower ? lt->getRgbPower() / lightDistrib.sum() : 1.0f / lights.size();
    return LightSample{ lt, pdf };
}

LightEnvSample Scene::sampleLightAndEnv(glm::vec2 u1, float u2)
{
    auto lightSample = sampleOneLight(u1);
    if (!lightSample)
        return { env, 1.0f };

    float pdfSampleLight = lightAndEnvStrategy == LightSampleStrategy::ByPower ?
            lightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    auto [lt, pdfLight] = lightSample.value();

    if (u2 > pdfSampleLight)
        return { env, 1.0f - pdfSampleLight };
    else
        return { lt, pdfLight * pdfSampleLight };
}

LiSample Scene::sampleLiOneLight(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2)
{
    if (lights.size() == 0)
        return InvalidLiSample;
    
    auto lightSample = sampleOneLight(u1);
    auto [lt, pdfSample] = lightSample.value();

    auto liSample = lt->sampleLi(x, u2);
    if (!liSample)
        return InvalidLiSample;

    auto [Wi, weight, dist, pdf] = liSample.value();

    Ray lightRay(x + Wi * 1e-4f, Wi);
    float testDist = dist - 1e-4f - 1e-6f;

    if (bvh->testIntersec(lightRay, testDist) || pdf < 1e-8f)
        return InvalidLiSample;

    pdf *= pdfSample;
    return {Wi, weight / pdf, pdf};
}

LiSample Scene::sampleLiEnv(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2)
{
    auto [Wi, weight, pdf] = env->sampleLi(u1, u2);

    Ray ray(x + Wi * 1e-4f, Wi);
    float tmp = 1e6;
    if (quickIntersect(ray, tmp))
        return InvalidLiSample;

    return { Wi, weight / pdf, pdf };
}

LiSample Scene::sampleLiLightAndEnv(const glm::vec3 &x, const std::array<float, 5> &sample)
{
    float pdfSampleLight = 0.0f;

    if (lights.size() > 0)
    {
        pdfSampleLight = lightAndEnvStrategy == LightSampleStrategy::ByPower ?
            lightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    }

    bool sampleLight = sample[0] < pdfSampleLight;
    float pdfSelect = sampleLight ? pdfSampleLight : 1.0f - pdfSampleLight;

    glm::vec2 u1(sample[1], sample[2]);
    glm::vec2 u2(sample[3], sample[4]);

    auto [Wi, coef, pdf] = sampleLight ? sampleLiOneLight(x, u1, u2) : sampleLiEnv(x, u1, u2);
    return {Wi, coef / pdfSelect, pdf * pdfSelect};
}

float Scene::pdfSampleLight(Light *lt)
{
    if (lights.size() == 0)
        return 0.0f;

    float fstPdf = lightSampleStrategy == LightSampleStrategy::ByPower ?
        lt->getRgbPower() / lightDistrib.sum() :
        1.0f / lights.size();

    float sndPdf = lightAndEnvStrategy == LightSampleStrategy::ByPower ?
        lightDistrib.sum() / powerlightAndEnv() :
        0.5f;

    return fstPdf * sndPdf;
}

float Scene::pdfSampleEnv()
{
    return lightAndEnvStrategy == LightSampleStrategy::ByPower ?
        env->power() / (lightDistrib.sum() + env->power()) :
        0.5f;
}

IiSample Scene::sampleIiCamera(glm::vec3 x, glm::vec2 u)
{
    auto [Wo, dist, imp, uv, pdf] = camera->sampleIi(x, u);

    Ray camRay(x + Wo * 1e-4f, Wo);
    float testDist = dist - 1e-4f - 1e-6f;

    if (bvh->testIntersec(camRay, testDist) || pdf < 1e-8f)
        return InvalidIiSample;
    return { Wo, imp / pdf, pdf };
}

void Scene::buildScene()
{
    bvh = std::make_shared<BVH>(hittables);
    //auto [maxDepth, avgDepth] = bvh->dfsDetailed();
    //std::cout << "[BVH] TreeSize: " << bvh->size() << "  MaxDepth: " << maxDepth << "  AvgDepth: " << avgDepth << "\n";
    setupLightSampleTable();
    box = bvh->box();
    boundRadius = glm::distance(box.pMin, box.pMax) * 0.5f;
}

void Scene::addLight(LightPtr light)
{
    lights.push_back(light);
    hittables.push_back(light);
}

void Scene::addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material)
{
    auto [vertices, texcoords, normals] = ObjReader::readFile(path);
    int faceCount = vertices.size() / 3;
    for (int i = 0; i < faceCount; i++)
    {
        glm::vec3 v[] = {vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]};
        glm::vec3 n[] = {normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]};
        glm::vec2 t[3];

        auto tr = std::make_shared<MeshTriangle>(v, t, n);
        tr->setTransform(transform);
        hittables.push_back(std::make_shared<Object>(tr, material));
    }
}

void Scene::addLightMesh(const char *path, TransformPtr transform, const glm::vec3 &power)
{
    auto [vertices, texcoords, normals] = ObjReader::readFile(path);
    int faceCount = vertices.size() / 3;
    for (int i = 0; i < faceCount; i++)
    {
        glm::vec3 v[] = {vertices[i * 3 + 0], vertices[i * 3 + 1], vertices[i * 3 + 2]};
        glm::vec3 n[] = {normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2]};
        glm::vec2 t[3];

        auto tr = std::make_shared<Light>(std::make_shared<MeshTriangle>(v, t, n), power, false);

        tr->setTransform(transform);
        hittables.push_back(tr);
        lights.push_back(tr);
    }
}