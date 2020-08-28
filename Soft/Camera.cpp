#include "Camera.h"

#include <Windows.h>

void Camera::move(glm::vec3 vect)
{
	m_Pos += vect;
}

void Camera::move(int key)
{
	switch (key)
	{
	case 'W':
		m_Pos.x += CAMERA_MOVE_SENSITIVITY * cos(glm::radians(m_Angle.x));
		m_Pos.y += CAMERA_MOVE_SENSITIVITY * sin(glm::radians(m_Angle.x));
		break;
	case 'S':
		m_Pos.x -= CAMERA_MOVE_SENSITIVITY * cos(glm::radians(m_Angle.x));
		m_Pos.y -= CAMERA_MOVE_SENSITIVITY * sin(glm::radians(m_Angle.x));
		break;
	case 'A':
		m_Pos.y += CAMERA_MOVE_SENSITIVITY * cos(glm::radians(m_Angle.x));
		m_Pos.x -= CAMERA_MOVE_SENSITIVITY * sin(glm::radians(m_Angle.x));
		break;
	case 'D':
		m_Pos.y -= CAMERA_MOVE_SENSITIVITY * cos(glm::radians(m_Angle.x));
		m_Pos.x += CAMERA_MOVE_SENSITIVITY * sin(glm::radians(m_Angle.x));
		break;
	case 'Q':
		roll(-CAMERA_ROLL_SENSITIVITY);
		break;
	case 'E':
		roll(CAMERA_ROLL_SENSITIVITY);
		break;
	case 'R':
		m_CameraUp = CAMERA_VEC_UP;
		break;
	case VK_SPACE:
		m_Pos.z += CAMERA_MOVE_SENSITIVITY;
		break;
	case VK_SHIFT:
		m_Pos.z -= CAMERA_MOVE_SENSITIVITY;
		break;
	}
	m_Pointing = pointing();
}

void Camera::roll(float angle)
{
	glm::mat4 mul(1.0f);
	mul = glm::rotate(mul, glm::radians(angle), m_Pointing);
	glm::vec4 tmp(m_CameraUp.x, m_CameraUp.y, m_CameraUp.z, 1.0f);
	tmp = mul * tmp;
	m_CameraUp = glm::vec3(tmp);
}

void Camera::rotate(glm::vec3 angle)
{
	m_Angle += angle * CAMERA_ROTATE_SENSITIVITY;
	if (m_Angle.y > CAMERA_PITCH_LIMIT) m_Angle.y = CAMERA_PITCH_LIMIT;
	if (m_Angle.y < -CAMERA_PITCH_LIMIT) m_Angle.y = -CAMERA_PITCH_LIMIT;
	m_Pointing = pointing();
}

void Camera::changeFOV(float offset)
{
	m_FOV -= glm::radians(offset) * CAMERA_FOV_SENSITIVITY;
	if (m_FOV > 90.0) m_FOV = 90.0;
	if (m_FOV < 15.0) m_FOV = 15.0;
}

void Camera::setFOV(float fov)
{
	m_FOV = fov;
	if (m_FOV > 90.0f) m_FOV = 90.0f;
	if (m_FOV < 15.0f) m_FOV = 15.0f;
}

void Camera::lookAt(glm::vec3 pos)
{
	setDir(pos - m_Pos);
}

void Camera::setDir(glm::vec3 dir)
{
	dir = glm::normalize(dir);
	m_Angle.y = glm::degrees(asin(dir.z / length(dir)));
	glm::vec2 dxy(dir);
	m_Angle.x = glm::degrees(asin(dir.y / length(dxy)));
	m_Pointing = pointing();
}

glm::vec3 Camera::pointing()
{
	float aX = cos(glm::radians(m_Angle.y)) * cos(glm::radians(m_Angle.x));
	float aY = cos(glm::radians(m_Angle.y)) * sin(glm::radians(m_Angle.x));
	float aZ = sin(glm::radians(m_Angle.y));
	return glm::normalize(glm::vec3(aX, aY, aZ));
}

glm::mat4 Camera::viewMatrix()
{
	float aX = cos(glm::radians(m_Angle.y)) * cos(glm::radians(m_Angle.x));
	float aY = cos(glm::radians(m_Angle.y)) * sin(glm::radians(m_Angle.x));
	float aZ = sin(glm::radians(m_Angle.y));
	glm::vec3 lookingAt = m_Pos + glm::vec3(aX, aY, aZ);
	glm::mat4 view = glm::lookAt(m_Pos, lookingAt, m_CameraUp);
	return view;
}

glm::mat4 Camera::viewMatrix(glm::vec3 focus) const
{
	glm::mat4 view = glm::lookAt(m_Pos, focus, m_CameraUp);
	return view;
}

glm::mat4 Camera::projMatrix(int width, int height) const
{
	glm::mat4 proj = glm::perspective(glm::radians(m_FOV), (float)width / (float)height, m_Near, m_Far);
	return proj;
}
