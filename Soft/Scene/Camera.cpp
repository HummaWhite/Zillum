#include "Camera.h"
#include <Windows.h>

Camera::Camera(glm::vec3 pos, glm::vec3 angle) :
    pos(pos), angle(angle)
{
    update();
}

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

void Camera::changeFOV(float offset)
{
    FOV -= glm::radians(offset) * CAMERA_FOV_SENSITIVITY;
    if (FOV > 90.0)
        FOV = 90.0;
    if (FOV < 15.0)
        FOV = 15.0;
}

void Camera::setFOV(float fov)
{
    FOV = fov;
    if (FOV > 90.0f)
        FOV = 90.0f;
    if (FOV < 15.0f)
        FOV = 15.0f;
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

Ray Camera::getRay(float x, float y)
{
    glm::vec3 rayDir = glm::normalize(front + (up * y + right * x * aspect) * (float)tan(glm::radians(FOV * 0.5f)));
    return {pos, glm::normalize(rayDir)};
}

glm::mat4 Camera::viewMatrix()
{
    float aX = cos(glm::radians(angle.y)) * cos(glm::radians(angle.x));
    float aY = cos(glm::radians(angle.y)) * sin(glm::radians(angle.x));
    float aZ = sin(glm::radians(angle.y));
    glm::vec3 lookingAt = pos + glm::vec3(aX, aY, aZ);
    glm::mat4 view = glm::lookAt(pos, lookingAt, up);
    return view;
}

glm::mat4 Camera::viewMatrix(glm::vec3 focus) const
{
    glm::mat4 view = glm::lookAt(pos, focus, up);
    return view;
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