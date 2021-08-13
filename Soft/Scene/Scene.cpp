#include "Scene.h"

Scene::Scene(const std::vector<HittablePtr> &hittables, EnvironmentPtr environment, CameraPtr camera) :
    hittables(hittables), env(environment), camera(camera)
{
    for (const auto &i : hittables)
    {
        if (i->type() == HittableType::Light)
        {
            lights.push_back(std::shared_ptr<Light>(dynamic_cast<Light *>(i.get())));
        }
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

LightSample Scene::sampleOneLight(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2)
{
    if (lights.size() == 0)
        return INVALID_LIGHT_SAMPLE;
    bool sampleByPower = lightSelectStrategy == LightSelectStrategy::ByPower;
    int index = sampleByPower ? lightDistrib.sample(u1) : static_cast<int>(lights.size() * u1.x);

    auto lt = lights[index];
    glm::vec3 y = lt->uniformSample(u2);
    glm::vec3 Wi = glm::normalize(y - x);
    glm::vec3 N = lt->surfaceNormal(y);
    float cosTheta = glm::dot(N, -Wi);

    if (cosTheta <= 1e-6f)
        return INVALID_LIGHT_SAMPLE;

    float dist = glm::distance(x, y);
    float pdf = dist * dist / (lt->surfaceArea() * cosTheta);

    Ray lightRay(x + Wi * 1e-4f, Wi);
    float testDist = dist - 1e-4f - 1e-6f;

    if (bvh->testIntersec(lightRay, testDist) || pdf < 1e-8f)
        return INVALID_LIGHT_SAMPLE;

    glm::vec3 weight = lt->getRadiance(y, -Wi);
    float pdfSample = sampleByPower ? lt->getRgbPower() / lightDistrib.sum() : 1.0f / lights.size();
    pdf *= pdfSample;
    return {Wi, weight / pdf, pdf};
}

LightSample Scene::sampleEnvironment(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2)
{
    auto [Wi, pdf] = env->importanceSample(u1, u2);

    Ray ray(x + Wi * 1e-4f, Wi);
    float tmp = 1e6;
    if (quickIntersect(ray, tmp))
        return INVALID_LIGHT_SAMPLE;

    auto rad = env->getRadiance(Wi);
    return {Wi, rad / pdf, pdf};
}

LightSample Scene::sampleLightAndEnv(const glm::vec3 &x, const std::array<float, 5> &sample)
{
    float pdfSampleLight = 0.0f;

    if (lights.size() > 0)
    {
        pdfSampleLight = lightAndEnvStrategy == LightSelectStrategy::ByPower ?
            lightDistrib.sum() / powerlightAndEnv() :
            0.5f;
    }

    bool sampleLight = sample[0] < pdfSampleLight;
    float pdfSelect = sampleLight ? pdfSampleLight : 1.0f - pdfSampleLight;

    glm::vec2 u1(sample[1], sample[2]);
    glm::vec2 u2(sample[3], sample[4]);

    auto [Wi, coef, pdf] = sampleLight ? sampleOneLight(x, u1, u2) : sampleEnvironment(x, u1, u2);
    return {Wi, coef / pdfSelect, pdf * pdfSelect};
}

float Scene::pdfSelectLight(Light *lt)
{
    float fstPdf = lightSelectStrategy == LightSelectStrategy::ByPower ?
        lt->getRgbPower() / lightDistrib.sum() :
        1.0f / lights.size();

    float sndPdf = lightAndEnvStrategy == LightSelectStrategy::ByPower ?
        lightDistrib.sum() / powerlightAndEnv() :
        0.5f;

    return fstPdf * sndPdf;
}

float Scene::pdfSelectEnv()
{
    return lightAndEnvStrategy == LightSelectStrategy::ByPower ?
        env->power() / (lightDistrib.sum() + env->power()) :
        0.5f;
}

void Scene::buildScene()
{
    bvh = std::make_shared<BVH>(hittables);
    //auto [maxDepth, avgDepth] = bvh->dfsDetailed();
    //std::cout << "[BVH] TreeSize: " << bvh->size() << "  MaxDepth: " << maxDepth << "  AvgDepth: " << avgDepth << "\n";
    setupLightSampleTable();
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