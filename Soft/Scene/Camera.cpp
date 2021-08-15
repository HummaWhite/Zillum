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

void Camera::update()
{
    float aX = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
    float aY = cos(glm::radians(angle.y)) * sin(glm::radians(angle.x));
    float aZ = sin(glm::radians(angle.y));
    front = glm::normalize(glm::vec3(aX, aY, aZ));
    glm::vec3 u(0.0f, 0.0f, 1.0f);
    right = glm::normalize(glm::cross(front, u));
    up = glm::normalize(glm::cross(right, front));
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
    dir = glm::normalize(right * dir.x + up * dir.y + front * dir.z);
    auto ori = pos + right * pLens.x + up * pLens.y;

    return { ori, dir };
}

CameraIiSample ThinLensCamera::sampleIi(glm::vec3 x, glm::vec2 u)
{
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
}