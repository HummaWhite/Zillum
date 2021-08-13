#include "Environments.h"

EnvSphereMapHDR::EnvSphereMapHDR(const char *filePath)
{
    sphereMap.loadFloat(filePath);
    w = sphereMap.texWidth();
    h = sphereMap.texHeight();

    float *pdf = new float[w * h];
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            pdf[j * w + i] = Math::rgbBrightness(sphereMap(i, j)) * glm::sin((float)(j + 0.5f) / h * Math::Pi);
        }
    }

    distrib = PiecewiseIndependent2D(pdf, w, h);
    delete[] pdf;
}

std::pair<glm::vec3, float> EnvSphereMapHDR::importanceSample(const glm::vec2 &u1, const glm::vec2 &u2)
{
    auto [col, row] = distrib.sample(u1, u2);

    float sinTheta = glm::sin(Math::Pi * (row + 0.5f) / h);
    auto Wi = Transform::planeToSphere(glm::vec2((col + 0.5f) / w, (row + 0.5f) / h));
    //float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) / sinTheta;
    float pdf = pdfLi(Wi);

    return {Wi, pdf};
}

float EnvSphereMapHDR::pdfLi(const glm::vec3 &Wi)
{
    float sinTheta = glm::asin(glm::abs(Wi.z));
    if (sinTheta < 1e-6f)
        return 0.0f;
    float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) /* sinTheta*/;

    return (pdf == 0.0f) ? 0.0f : pdf;
}

float EnvSphereMapHDR::getPortion(const glm::vec3 &Wi)
{
    return Math::rgbBrightness(glm::vec3(sphereMap.getSpherical(Wi))) / distrib.sum();
}

glm::vec3 EnvTest::getRadiance(const glm::vec3 &dir)
{
    glm::vec2 uv = Transform::sphereToPlane(dir);

    int r = (int)(uv.x * row);
    int c = (int)(uv.y * col);

    return (r & 1) ^ (c & 1) ? radiance : glm::vec3(0.0f);
}