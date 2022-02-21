#include "../../../include/Core/Environment.h"
#include "../../../include/Utils/ImageSave.h"

EnvSphereMapHDR::EnvSphereMapHDR(const char *filePath)
{
    mSphereMap.loadFloat(filePath);
    mWidth = mSphereMap.texWidth();
    mHeight = mSphereMap.texHeight();

    float *pdf = new float[mWidth * mHeight];
    float sum = 0.0f;
    for (int j = 0; j < mHeight; j++)
    {
        for (int i = 0; i < mWidth; i++)
        {
            pdf[j * mWidth + i] = Math::luminance(mSphereMap(i, j)) * glm::sin((float)(j + 0.5f) / mHeight * Math::Pi);
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
    auto Wi = Transform::planeToSphere(Vec2f((col + 0.5f) / mWidth, (row + 0.5f) / mHeight));
    //float pdf = getPortion(Wi) * float(w * h) * 0.5f * Math::square(Math::PiInv) / sinTheta;
    float pdf = pdfLi(Wi);

    return { Wi, radiance(Wi), pdf };
}

float EnvSphereMapHDR::pdfLi(const Vec3f &Wi)
{
    float sinTheta = glm::asin(glm::abs(Wi.z));
    if (sinTheta < 1e-6f)
        return 0.0f;
    float pdf = getPortion(Wi) * float(mWidth * mHeight) * 0.5f * Math::square(Math::PiInv) /* sinTheta*/;

    return (pdf == 0.0f) ? 0.0f : pdf;
}

float EnvSphereMapHDR::getPortion(const Vec3f &Wi)
{
    return Math::luminance(Spectrum(mSphereMap.getSpherical(Wi))) / mDistrib.sum();
}