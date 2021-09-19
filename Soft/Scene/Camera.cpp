#include "Camera.h"
#include <Windows.h>

void Camera::move(int key)
{
    switch (key)
    {
    case 'W':
        pos.x += CameraMoveSensitivity * cos(glm::radians(angle.x));
        pos.y += CameraMoveSensitivity * sin(glm::radians(angle.x));
        break;
    case 'S':
        pos.x -= CameraMoveSensitivity * cos(glm::radians(angle.x));
        pos.y -= CameraMoveSensitivity * sin(glm::radians(angle.x));
        break;
    case 'A':
        pos.y += CameraMoveSensitivity * cos(glm::radians(angle.x));
        pos.x -= CameraMoveSensitivity * sin(glm::radians(angle.x));
        break;
    case 'D':
        pos.y -= CameraMoveSensitivity * cos(glm::radians(angle.x));
        pos.x += CameraMoveSensitivity * sin(glm::radians(angle.x));
        break;
    case 'Q':
        roll(-CameraRollSensitivity);
        break;
    case 'E':
        roll(CameraRollSensitivity);
        break;
    case 'R':
        up = Vec3f(0.0f, 0.0f, 1.0f);
        break;
    case VK_SPACE:
        pos.z += CameraMoveSensitivity;
        break;
    case VK_SHIFT:
        pos.z -= CameraMoveSensitivity;
        break;
    }
    update();
}

void Camera::roll(float rolAngle)
{
    glm::mat4 mul(1.0f);
    mul = glm::rotate(mul, glm::radians(rolAngle), front);
    glm::vec4 tmp(up.x, up.y, up.z, 1.0f);
    tmp = mul * tmp;
    up = Vec3f(tmp);
    update();
}

void Camera::rotate(Vec3f rotAngle)
{
    angle += rotAngle * CameraRotateSensitivity;
    if (angle.y > CameraPitchSensitivity)
        angle.y = CameraPitchSensitivity;
    if (angle.y < -CameraPitchSensitivity)
        angle.y = -CameraPitchSensitivity;
    update();
}

void Camera::setDir(Vec3f dir)
{
    dir = glm::normalize(dir);
    angle.y = glm::degrees(asin(dir.z / length(dir)));
    Vec2f dxy(dir);
    angle.x = glm::degrees(asin(dir.y / length(dxy)));
    if (dir.x < 0)
        angle.x = 180.0f - angle.x;
    update();
}

void Camera::setAngle(Vec3f ang)
{
    angle = ang;
    update();
}

bool Camera::inFilmBound(Vec2f p)
{
    return (p.x >= 0.0f && p.x <= Math::OneMinusEpsilon && p.y >= 0.0f && p.y <= Math::OneMinusEpsilon);
}

void Camera::update()
{
    float aX = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
    float aY = cos(glm::radians(angle.y)) * sin(glm::radians(angle.x));
    float aZ = sin(glm::radians(angle.y));

    front = glm::normalize(Vec3f(aX, aY, aZ));
    right = glm::normalize(glm::cross(front, Vec3f(0.0f, 0.0f, 1.0f)));
    up = glm::normalize(glm::cross(right, front));
    
    tbnMat = Mat3f(right, up, front);
    tbnInv = glm::inverse(tbnMat);
}

Vec2f ThinLensCamera::getRasterPos(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    float dFocus = (approxPinhole ? 1.0f : focalDist) / cosTheta;
    Vec3f pFocus = tbnInv * (ray.get(dFocus) - pos);

    Vec2f filmSize(film.width, film.height);
    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV_ * 0.5f));

    pFocus /= Vec3f(Vec2f(aspect, 1.0f) * tanFOV, 1.0f) * focalDist;
    Vec2f ndc(pFocus);
    ndc.y = -ndc.y;
    return (ndc + 1.0f) * 0.5f;
}

Ray ThinLensCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2D(), sampler);
}

Ray ThinLensCamera::generateRay(Vec2f uv, SamplerPtr sampler)
{
    Vec2f filmSize(film.width, film.height);
    auto texelSize = Vec2f(1.0f) / filmSize;
    auto biased = uv + texelSize * sampler->get2D();
    auto ndc = biased;

    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV_ * 0.5f));

    Vec3f pLens(Transform::toConcentricDisk(sampler->get2D()) * lensRadius, 0.0f);
    Vec3f pFocusPlane(ndc * Vec2f(aspect, 1.0f) * focalDist * tanFOV, focalDist);

    auto dir = pFocusPlane - pLens;
    dir = glm::normalize(tbnMat * dir);
    auto ori = pos + tbnMat * pLens;

    return { ori, dir };
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

CameraIiSample ThinLensCamera::sampleIi(Vec3f ref, Vec2f u)
{
    Vec3f pLens(Transform::toConcentricDisk(u) * lensRadius, 0.0f);
    Vec3f y = pos + tbnMat * pLens;
    float dist = glm::distance(ref, y);

    Vec3f Wi = glm::normalize(y - ref);
    float cosTheta = Math::satDot(front, -Wi);
    if (cosTheta < 1e-6f)
        return InvalidCamIiSample;

    Ray ray(y, -Wi);
    Vec2f uv = getRasterPos(ray);
    float pdf = dist * dist / (cosTheta * (approxPinhole ? 1.0f : lensArea));

    return { Wi, Ie(ray), dist, uv, pdf };
}

std::pair<float, float> ThinLensCamera::pdfIe(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return { 0.0f, 0.0f };
    
    Vec2f pRaster = getRasterPos(ray);

    if (!inFilmBound(pRaster))
        return { 0.0f, 0.0f };
    
    float pdfPos = approxPinhole ? 1.0f : 1.0f / lensArea;
    float pdfDir = 1.0f / Math::qpow(cosTheta, 3);
    return { pdfPos, pdfDir };
}

Vec3f ThinLensCamera::Ie(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return Vec3f(0.0f);

    Vec2f pRaster = getRasterPos(ray);

    if (!inFilmBound(pRaster))
        return Vec3f(0.0f);

    return Vec3f(1.0f / Math::qpow(cosTheta, 4)) / (approxPinhole ? 1.0f : lensArea);
}

Vec2f PanoramaCamera::getRasterPos(Ray ray)
{
    return {};
}

Ray PanoramaCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2D(), sampler);
}

Ray PanoramaCamera::generateRay(Vec2f uv, SamplerPtr sampler)
{
    auto dir = Transform::planeToSphere((uv + Vec2f(0.5f, 1.0f)) * 0.5f);
    dir.x = -dir.x;
    dir.z = -dir.z;
    return { pos, dir };
}

CameraIiSample PanoramaCamera::sampleIi(Vec3f ref, Vec2f u)
{
    return {};
}

std::pair<float, float> PanoramaCamera::pdfIe(Ray ray)
{
    return {};
}

Vec3f PanoramaCamera::Ie(Ray ray)
{
    return {};
}