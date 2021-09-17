#include "Environments.h"

EnvLiSample EnvSingleColor::sampleLi(const Vec2f &u1, const Vec2f &u2)
{
    auto Wi = Transform::planeToSphere(u1);
    return { Wi, radiance, Math::PiInv * 0.25f };
}

float EnvSingleColor::pdfLi(const Vec3f &Wi)
{
    return Math::PiInv * 0.25f;
}

LightLeSample EnvSingleColor::sampleLe(float radius, const std::array<float, 6> &u)
{
    auto Wi = Transform::planeToSphere({ u[0], u[1] });
    auto ori = Vec3f(Transform::toConcentricDisk({ u[2], u[3] }), 1.0f) * radius;
    ori = Transform::normalToWorld(Wi, ori);

    float pdf = Math::PiInv * 0.25f * Math::PiInv * Math::square(1.0f / radius);
    return { { ori, -Wi }, radiance, pdf };
}

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

EnvLiSample EnvSphereMapHDR::sampleLi(const Vec2f &u1, const Vec2f &u2)
{
    auto [col, row] = distrib.sample(u1, u2);

    float sinTheta = glm::sin(Math::Pi * (row + 0.5f) / h);
    auto Wi = Transform::planeToSphere(Vec2f((col + 0.5f) / w, (row + 0.5f) / h));
    //float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) / sinTheta;
    float pdf = pdfLi(Wi);

    return { Wi, getRadiance(Wi), pdf };
}

float EnvSphereMapHDR::pdfLi(const Vec3f &Wi)
{
    float sinTheta = glm::asin(glm::abs(Wi.z));
    if (sinTheta < 1e-6f)
        return 0.0f;
    float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) /* sinTheta*/;

    return (pdf == 0.0f) ? 0.0f : pdf;
}

float EnvSphereMapHDR::getPortion(const Vec3f &Wi)
{
    return Math::rgbBrightness(Vec3f(sphereMap.getSpherical(Wi))) / distrib.sum();
}

LightLeSample EnvSphereMapHDR::sampleLe(float radius, const std::array<float, 6> &u)
{
    auto [Wi, radiance, pdfDir] = sampleLi({ u[0], u[1] }, { u[2], u[3] });
    auto ori = Vec3f(Transform::toConcentricDisk({ u[4], u[5] }), 1.0f) * radius;
    ori = Transform::normalToWorld(Wi, ori);

    float pdf = pdfDir * Math::PiInv * Math::square(1.0f / radius);
    return { { ori, -Wi }, radiance, pdf };
}