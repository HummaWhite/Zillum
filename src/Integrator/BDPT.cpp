#include "../../include/Core/Integrator.h"

struct BDPTVertex
{
    static BDPTVertex createCamera();

    static BDPTVertex createSurface();

    static BDPTVertex createEnvLight();

    static BDPTVertex createAreaLight();

    Vec3f P;
    Vec3f N;
    Vec3f Wo;
    MaterialPtr material;

    float pdfFwd;
    float pdfBwd;
};

Spectrum BDPTIntegrator::tracePixel(Ray ray, SamplerPtr sampler)
{
    auto lightPath = createLightPath(sampler);
    auto cameraPath = createCameraPath(ray, sampler);
    return eval(lightPath, cameraPath);
}

std::vector<BDPTVertex> BDPTIntegrator::createLightPath(SamplerPtr sampler)
{
    std::vector<BDPTVertex> vertices;


    return vertices;
}

std::vector<BDPTVertex> BDPTIntegrator::createCameraPath(const Ray &ray, SamplerPtr sampler)
{
    std::vector<BDPTVertex> vertices;


    return vertices;
}

Spectrum BDPTIntegrator::eval(const std::vector<BDPTVertex> &lightPath, const std::vector<BDPTVertex> &cameraPath)
{
    Spectrum result(0.0f);

    int lightLength = lightPath.size();
    int cameraLength = cameraPath.size();

    for (int s = 0; s < lightLength; s++)
    {
        for (int t = 1; t < cameraLength; t++)
        {
        }
    }
    return result;
}