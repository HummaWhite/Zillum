#include "../../../include/Core/Environment.h"
#include "../../../include/Utils/ImageSave.h"

EnvSphereMapHDR::EnvSphereMapHDR(const char *filePath)
{
    mSphereMap = TextureLoader::fromF32x3(filePath);
    mWidth = mSphereMap->texWidth();
    mHeight = mSphereMap->texHeight();

    float *pdf = new float[mWidth * mHeight];
    float sum = 0.0f;
    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            pdf[j * mWidth + i] = Math::luminance((*mSphereMap)(i, j)) * glm::sin((float)(j + 0.5f) / mHeight * Math::Pi);
            sum += pdf[j * mWidth + i];
        }
    }
    mDistrib = PiecewiseIndependent2D(pdf, mWidth, mHeight);
    delete[] pdf;
}

EnvLiSample EnvSphereMapHDR::sampleLi(const Vec2f &u1, const Vec2f &u2)
{
    auto [col, row] = mDistrib.sample(u1, u2);

    float sinTheta = glm::sin(Math::Pi * (row + 0.5f) / mHeight);
    auto wi = Transform::planeToSphere(Vec2f((col + 0.5f) / mWidth, (row + 0.5f) / mHeight));
    //float pdf = getPortion(wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) / sinTheta;
    float pdf = pdfLi(wi);

    return { wi, radiance(wi), pdf };
}

float EnvSphereMapHDR::pdfLi(const Vec3f &wi)
{
    float sinTheta = glm::asin(glm::abs(wi.z));
    if (sinTheta < 1e-6f)
        return 0.0f;
    float pdf = getPortion(wi) * float(mWidth * mHeight) * 0.5f * Math::square(Math::PiInv) /* sinTheta*/;

    return (pdf == 0.0f) ? 0.0f : pdf;
}

float EnvSphereMapHDR::getPortion(const Vec3f &wi)
{
    return Math::luminance(Spectrum(mSphereMap->getSpherical(wi))) / mDistrib.sum();
}