#include "../../../include/Core/Camera.h"

Vec2f ThinLensCamera::rasterPos(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, mFront);
    float dFocus = (mIsDelta ? 1.0f : mFocalDist) / cosTheta;
    Vec3f pFocus = mTBNInv * (ray.get(dFocus) - mPos);

    Vec2f filmSize(mFilm.width, mFilm.height);
    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(mFOV * 0.5f));

    pFocus /= Vec3f(Vec2f(aspect, 1.0f) * tanFOV, 1.0f) * mFocalDist;
    Vec2f ndc(pFocus);
    ndc.y = -ndc.y;
    return (ndc + 1.0f) * 0.5f;
}

Ray ThinLensCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2(), sampler);
}

Ray ThinLensCamera::generateRay(Vec2f uv, SamplerPtr sampler)
{
    Vec2f filmSize(mFilm.width, mFilm.height);
    auto texelSize = Vec2f(1.0f) / filmSize;
    auto biased = uv + texelSize * sampler->get2();
    auto ndc = biased;

    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(mFOV * 0.5f));

    Vec3f pLens(Transform::toConcentricDisk(sampler->get2()) * mLensRadius, 0.0f);
    Vec3f pFocusPlane(ndc * Vec2f(aspect, 1.0f) * mFocalDist * tanFOV, mFocalDist);

    auto dir = pFocusPlane - pLens;
    dir = glm::normalize(mTBNMat * dir);
    auto ori = mPos + mTBNMat * pLens;

    return { ori, dir };
}

float ThinLensCamera::pdfIi(Vec3f ref, Vec3f y)
{
    if (mIsDelta)
        return 0.0f;
    auto Wi = glm::normalize(y - ref);

    float cosTheta = Math::satDot(mFront, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;
    return Math::distSquare(ref, y) / (mLensArea * cosTheta);
}

std::optional<CameraIiSample> ThinLensCamera::sampleIi(Vec3f ref, Vec2f u)
{
    Vec3f pLens(Transform::toConcentricDisk(u) * mLensRadius, 0.0f);
    Vec3f y = mPos + mTBNMat * pLens;
    float dist = glm::distance(ref, y);

    Vec3f wi = glm::normalize(y - ref);
    float cosTheta = Math::satDot(mFront, -wi);
    if (cosTheta < 1e-6f)
        return std::nullopt;

    Ray ray(y, -wi);
    Vec2f uv = rasterPos(ray);
    float pdf = dist * dist / (cosTheta * (mIsDelta ? 1.0f : mLensArea));

    return CameraIiSample{ wi, Ie(ray), dist, uv, pdf };
}

CameraPdf ThinLensCamera::pdfIe(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, mFront);
    if (cosTheta < 1e-6f)
        return { 0.0f, 0.0f };

    Vec2f pRaster = rasterPos(ray);

    if (!inFilmBound(pRaster))
        return { 0.0f, 0.0f };

    float pdfPos = mIsDelta ? 1.0f : 1.0f / mLensArea;
    float pdfDir = 1.0f / (cosTheta * cosTheta * cosTheta);
    return { pdfPos, pdfDir };
}

Spectrum ThinLensCamera::Ie(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, mFront);
    if (cosTheta < 1e-6f)
        return Spectrum(0.0f);
        
    Vec2f pRaster = rasterPos(ray);
    if (!inFilmBound(pRaster))
        return Spectrum(0.0f);

    float tanFOVInv = 1.0f / glm::tan(glm::radians(mFOV * 0.5f));
    float cos2Theta = cosTheta * cosTheta;
    return Spectrum(tanFOVInv * tanFOVInv * 0.25f) / ((mIsDelta ? 1.0f : mLensArea) * cos2Theta * cos2Theta);
}