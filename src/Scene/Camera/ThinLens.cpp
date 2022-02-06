#include "../../../include/Core/Camera.h"

Vec2f ThinLensCamera::getRasterPos(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    float dFocus = (approxPinhole ? 1.0f : focalDist) / cosTheta;
    Vec3f pFocus = tbnInv * (ray.get(dFocus) - pos);

    Vec2f filmSize(film.width, film.height);
    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV * 0.5f));

    pFocus /= Vec3f(Vec2f(aspect, 1.0f) * tanFOV, 1.0f) * focalDist;
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
    Vec2f filmSize(film.width, film.height);
    auto texelSize = Vec2f(1.0f) / filmSize;
    auto biased = uv + texelSize * sampler->get2();
    auto ndc = biased;

    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV * 0.5f));

    Vec3f pLens(Transform::toConcentricDisk(sampler->get2()) * lensRadius, 0.0f);
    Vec3f pFocusPlane(ndc * Vec2f(aspect, 1.0f) * focalDist * tanFOV, focalDist);

    auto dir = pFocusPlane - pLens;
    dir = glm::normalize(tbnMat * dir);
    auto ori = pos + tbnMat * pLens;

    return {ori, dir};
}

float ThinLensCamera::pdfIi(Vec3f ref, Vec3f y)
{
    if (approxPinhole)
        return 0.0f;
    auto Wi = glm::normalize(y - ref);

    float cosTheta = Math::satDot(front, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;
    return Math::distSquare(ref, y) / (lensArea * cosTheta);
}

std::optional<CameraIiSample> ThinLensCamera::sampleIi(Vec3f ref, Vec2f u)
{
    Vec3f pLens(Transform::toConcentricDisk(u) * lensRadius, 0.0f);
    Vec3f y = pos + tbnMat * pLens;
    float dist = glm::distance(ref, y);

    Vec3f Wi = glm::normalize(y - ref);
    float cosTheta = Math::satDot(front, -Wi);
    if (cosTheta < 1e-6f)
        return std::nullopt;

    Ray ray(y, -Wi);
    Vec2f uv = getRasterPos(ray);
    float pdf = dist * dist / (cosTheta * (approxPinhole ? 1.0f : lensArea));

    return CameraIiSample{ Wi, Ie(ray), dist, uv, pdf };
}

std::pair<float, float> ThinLensCamera::pdfIe(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return {0.0f, 0.0f};

    Vec2f pRaster = getRasterPos(ray);

    if (!inFilmBound(pRaster))
        return {0.0f, 0.0f};

    float pdfPos = approxPinhole ? 1.0f : 1.0f / lensArea;
    float pdfDir = 1.0f / (cosTheta * cosTheta * cosTheta);
    return { pdfPos, pdfDir };
}

Vec3f ThinLensCamera::Ie(Ray ray)
{
    return Vec3f(1.0f);
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return Vec3f(0.0f);
    Vec2f pRaster = getRasterPos(ray);
    if (!inFilmBound(pRaster))
        return Vec3f(0.0f);
    
    float cos2Theta = cosTheta * cosTheta;

    return Vec3f(1.0f) / ((approxPinhole ? 1.0f : lensArea) * cos2Theta * cos2Theta);
}