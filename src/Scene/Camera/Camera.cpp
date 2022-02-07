#include <Windows.h>

#include "../../../include/Core/Camera.h"

void Camera::move(int key)
{
    switch (key)
    {
    case 'W':
        mPos.x += CameraMoveSensitivity * cos(glm::radians(mAngle.x));
        mPos.y += CameraMoveSensitivity * sin(glm::radians(mAngle.x));
        break;
    case 'S':
        mPos.x -= CameraMoveSensitivity * cos(glm::radians(mAngle.x));
        mPos.y -= CameraMoveSensitivity * sin(glm::radians(mAngle.x));
        break;
    case 'A':
        mPos.y += CameraMoveSensitivity * cos(glm::radians(mAngle.x));
        mPos.x -= CameraMoveSensitivity * sin(glm::radians(mAngle.x));
        break;
    case 'D':
        mPos.y -= CameraMoveSensitivity * cos(glm::radians(mAngle.x));
        mPos.x += CameraMoveSensitivity * sin(glm::radians(mAngle.x));
        break;
    case 'Q':
        roll(-CameraRollSensitivity);
        break;
    case 'E':
        roll(CameraRollSensitivity);
        break;
    case 'R':
        mUp = Vec3f(0.0f, 0.0f, 1.0f);
        break;
    case VK_SPACE:
        mPos.z += CameraMoveSensitivity;
        break;
    case VK_SHIFT:
        mPos.z -= CameraMoveSensitivity;
        break;
    }
    update();
}

void Camera::roll(float rolAngle)
{
    glm::mat4 mul(1.0f);
    mul = glm::rotate(mul, glm::radians(rolAngle), mFront);
    glm::vec4 tmp(mUp.x, mUp.y, mUp.z, 1.0f);
    tmp = mul * tmp;
    mUp = Vec3f(tmp);
    update();
}

void Camera::rotate(Vec3f rotAngle)
{
    mAngle += rotAngle * CameraRotateSensitivity;
    if (mAngle.y > CameraPitchSensitivity)
        mAngle.y = CameraPitchSensitivity;
    if (mAngle.y < -CameraPitchSensitivity)
        mAngle.y = -CameraPitchSensitivity;
    update();
}

void Camera::setDir(Vec3f dir)
{
    dir = glm::normalize(dir);
    mAngle.y = glm::degrees(asin(dir.z / length(dir)));
    Vec2f dxy(dir);
    mAngle.x = glm::degrees(asin(dir.y / length(dxy)));
    if (dir.x < 0)
        mAngle.x = 180.0f - mAngle.x;
    update();
}

void Camera::setAngle(Vec3f ang)
{
    mAngle = ang;
    update();
}

bool Camera::inFilmBound(Vec2f p)
{
    return (p.x >= 0.0f && p.x <= Math::OneMinusEpsilon && p.y >= 0.0f && p.y <= Math::OneMinusEpsilon);
}

void Camera::update()
{
    float aX = cos(glm::radians(mAngle.y)) * cos(glm::radians(mAngle.x));
    float aY = cos(glm::radians(mAngle.y)) * sin(glm::radians(mAngle.x));
    float aZ = sin(glm::radians(mAngle.y));

    mFront = glm::normalize(Vec3f(aX, aY, aZ));
    mRight = glm::normalize(glm::cross(mFront, Vec3f(0.0f, 0.0f, 1.0f)));
    mUp = glm::normalize(glm::cross(mRight, mFront));
    
    mTBNMat = Mat3f(mRight, mUp, mFront);
    mTBNInv = glm::inverse(mTBNMat);
}