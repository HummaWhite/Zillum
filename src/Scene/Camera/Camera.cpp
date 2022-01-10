#include <Windows.h>

#include "../../../include/Core/Camera.h"

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