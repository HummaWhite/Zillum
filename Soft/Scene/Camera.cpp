#include "Camera.h"
#include <Windows.h>

void Camera::move(int key)
{
    switch (key)
    {
    case 'W':
        pos.x += CAMERA_MOVE_SENSITIVITY * cos(glm::radians(angle.x));
        pos.y += CAMERA_MOVE_SENSITIVITY * sin(glm::radians(angle.x));
        break;
    case 'S':
        pos.x -= CAMERA_MOVE_SENSITIVITY * cos(glm::radians(angle.x));
        pos.y -= CAMERA_MOVE_SENSITIVITY * sin(glm::radians(angle.x));
        break;
    case 'A':
        pos.y += CAMERA_MOVE_SENSITIVITY * cos(glm::radians(angle.x));
        pos.x -= CAMERA_MOVE_SENSITIVITY * sin(glm::radians(angle.x));
        break;
    case 'D':
        pos.y -= CAMERA_MOVE_SENSITIVITY * cos(glm::radians(angle.x));
        pos.x += CAMERA_MOVE_SENSITIVITY * sin(glm::radians(angle.x));
        break;
    case 'Q':
        roll(-CAMERA_ROLL_SENSITIVITY);
        break;
    case 'E':
        roll(CAMERA_ROLL_SENSITIVITY);
        break;
    case 'R':
        up = glm::vec3(0.0f, 0.0f, 1.0f);
        break;
    case VK_SPACE:
        pos.z += CAMERA_MOVE_SENSITIVITY;
        break;
    case VK_SHIFT:
        pos.z -= CAMERA_MOVE_SENSITIVITY;
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
    up = glm::vec3(tmp);
    update();
}

void Camera::rotate(glm::vec3 rotAngle)
{
    angle += rotAngle * CAMERA_ROTATE_SENSITIVITY;
    if (angle.y > CAMERA_PITCH_LIMIT)
        angle.y = CAMERA_PITCH_LIMIT;
    if (angle.y < -CAMERA_PITCH_LIMIT)
        angle.y = -CAMERA_PITCH_LIMIT;
    update();
}

void Camera::setDir(glm::vec3 dir)
{
    dir = glm::normalize(dir);
    angle.y = glm::degrees(asin(dir.z / length(dir)));
    glm::vec2 dxy(dir);
    angle.x = glm::degrees(asin(dir.y / length(dxy)));
    if (dir.x < 0)
        angle.x = 180.0f - angle.x;
    update();
}

void Camera::setAngle(glm::vec3 ang)
{
    angle = ang;
    update();
}

bool Camera::inFilmBound(glm::vec2 p)
{
    return (p.x >= 0.0f && p.x <= Math::OneMinusEpsilon && p.y >= 0.0f && p.y <= Math::OneMinusEpsilon);
}

void Camera::update()
{
    float aX = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
    float aY = cos(glm::radians(angle.y)) * sin(glm::radians(angle.x));
    float aZ = sin(glm::radians(angle.y));

    front = glm::normalize(glm::vec3(aX, aY, aZ));
    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)));
    up = glm::normalize(glm::cross(right, front));
    
    tbnMat = glm::mat3(right, up, front);
    tbnInv = glm::inverse(tbnMat);
}

glm::vec2 ThinLensCamera::getRasterPos(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    float dFocus = (approxPinhole ? 1.0f : focalDist) / cosTheta;
    glm::vec3 pFocus = tbnInv * (ray.get(dFocus) - pos);

    glm::vec2 filmSize(film.width, film.height);
    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV * 0.5f));

    pFocus /= glm::vec3(glm::vec2(aspect, 1.0f) * tanFOV, 1.0f) * focalDist;
    glm::vec2 ndc(pFocus);
    ndc.y = -ndc.y;
    return (ndc + 1.0f) * 0.5f;
}

Ray ThinLensCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2D(), sampler);
}

Ray ThinLensCamera::generateRay(glm::vec2 uv, SamplerPtr sampler)
{
    glm::vec2 filmSize(film.width, film.height);
    auto texelSize = glm::vec2(1.0f) / filmSize;
    auto biased = uv + texelSize * sampler->get2D();
    auto ndc = biased;

    float aspect = filmSize.x / filmSize.y;
    float tanFOV = glm::tan(glm::radians(FOV * 0.5f));

    glm::vec3 pLens(Transform::toConcentricDisk(sampler->get2D()) * lensRadius, 0.0f);
    glm::vec3 pFocusPlane(ndc * glm::vec2(aspect, 1.0f) * focalDist * tanFOV, focalDist);

    auto dir = pFocusPlane - pLens;
    dir = glm::normalize(tbnMat * dir);
    auto ori = pos + tbnMat * pLens;

    return { ori, dir };
}

float ThinLensCamera::pdfIi(glm::vec3 x, glm::vec3 y)
{
    if (approxPinhole)
        return 0.0f;
    auto N = front;
    auto Wi = glm::normalize(y - x);

    float cosTheta = Math::satDot(N, -Wi);
    if (cosTheta < 1e-8f)
        return 0.0f;
    return Math::distSquare(x, y) / (lensArea * cosTheta);
}

CameraIiSample ThinLensCamera::sampleIi(glm::vec3 x, glm::vec2 u)
{
    glm::vec3 pLens(Transform::toConcentricDisk(u) * lensRadius, 0.0f);
    glm::vec3 y = pos + tbnMat * pLens;
    float dist = glm::distance(x, y);

    glm::vec3 Wi = glm::normalize(y - x);
    float cosTheta = Math::satDot(front, -Wi);
    if (cosTheta < 1e-6f)
        return InvalidCamIiSample;

    Ray ray(y, -Wi);
    glm::vec2 uv = getRasterPos(ray);
    float pdf = dist * dist / (cosTheta * (approxPinhole ? 1.0f : lensArea));

    return { Wi, Ie(ray), dist, uv, pdf };
}

std::pair<float, float> ThinLensCamera::pdfIe(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return { 0.0f, 0.0f };
    
    glm::vec2 pRaster = getRasterPos(ray);

    if (!inFilmBound(pRaster))
        return { 0.0f, 0.0f };
    
    float pdfPos = approxPinhole ? 1.0f : 1.0f / lensArea;
    float pdfDir = 1.0f / Math::qpow(cosTheta, 3);
    return { pdfPos, pdfDir };
}

glm::vec3 ThinLensCamera::Ie(Ray ray)
{
    float cosTheta = glm::dot(ray.dir, front);
    if (cosTheta < 1e-6f)
        return glm::vec3(0.0f);

    glm::vec2 pRaster = getRasterPos(ray);

    if (!inFilmBound(pRaster))
        return glm::vec3(0.0f);

    return glm::vec3(1.0f / Math::qpow(cosTheta, 4)) / (approxPinhole ? 1.0f : lensArea);
}

glm::vec2 PanoramaCamera::getRasterPos(Ray ray)
{
    return {};
}

Ray PanoramaCamera::generateRay(SamplerPtr sampler)
{
    return generateRay(sampler->get2D(), sampler);
}

Ray PanoramaCamera::generateRay(glm::vec2 uv, SamplerPtr sampler)
{
    auto dir = Transform::planeToSphere((uv + glm::vec2(0.5f, 1.0f)) * 0.5f);
    dir.x = -dir.x;
    dir.z = -dir.z;
    return { pos, dir };
}

CameraIiSample PanoramaCamera::sampleIi(glm::vec3 x, glm::vec2 u)
{
    return {};
}

std::pair<float, float> PanoramaCamera::pdfIe(Ray ray)
{
    return {};
}

glm::vec3 PanoramaCamera::Ie(Ray ray)
{
    return {};
}