#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

const float CAMERA_ROTATE_SENSITIVITY = 0.5f;
const float CAMERA_MOVE_SENSITIVITY = 0.1f;
const float CAMERA_ROLL_SENSITIVITY = 0.05f;
const float CAMERA_FOV_SENSITIVITY = 150.0f;
const float CAMERA_PITCH_LIMIT = 88.0f;

class Camera
{
public:
	Camera(glm::vec3 pos = { 0.0f, 0.0f, 0.0f }, glm::vec3 angle = { 90.0f, 0.0f, 0.0f }):
		m_Pos(pos), m_Angle(angle)
	{
		update();
	}

	void move(glm::vec3 vect) { m_Pos += vect; }

	void move(int key)
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
				m_Up = glm::vec3(0.0f, 0.0f, 1.0f);
				break;
			case VK_SPACE:
				m_Pos.z += CAMERA_MOVE_SENSITIVITY;
				break;
			case VK_SHIFT:
				m_Pos.z -= CAMERA_MOVE_SENSITIVITY;
				break;
		}
		update();
	}

	void roll(float angle)
	{
		glm::mat4 mul(1.0f);
		mul = glm::rotate(mul, glm::radians(angle), m_Front);
		glm::vec4 tmp(m_Up.x, m_Up.y, m_Up.z, 1.0f);
		tmp = mul * tmp;
		m_Up = glm::vec3(tmp);
		update();
	}

	void rotate(glm::vec3 angle)
	{
		m_Angle += angle * CAMERA_ROTATE_SENSITIVITY;
		if (m_Angle.y > CAMERA_PITCH_LIMIT) m_Angle.y = CAMERA_PITCH_LIMIT;
		if (m_Angle.y < -CAMERA_PITCH_LIMIT) m_Angle.y = -CAMERA_PITCH_LIMIT;
		update();
	}

	void changeFOV(float offset)
	{
		m_FOV -= glm::radians(offset) * CAMERA_FOV_SENSITIVITY;
		if (m_FOV > 90.0) m_FOV = 90.0;
		if (m_FOV < 15.0) m_FOV = 15.0;
	}

	void setFOV(float fov)
	{
		m_FOV = fov;
		if (m_FOV > 90.0f) m_FOV = 90.0f;
		if (m_FOV < 15.0f) m_FOV = 15.0f;
	}

	void lookAt(glm::vec3 pos)
	{
		setDir(pos - m_Pos);
	}

	void setDir(glm::vec3 dir)
	{
		dir = glm::normalize(dir);
		m_Angle.y = glm::degrees(asin(dir.z / length(dir)));
		glm::vec2 dxy(dir);
		m_Angle.x = glm::degrees(asin(dir.y / length(dxy)));
		update();
	}

	void setPos(glm::vec3 pos) { m_Pos = pos; }

	void setAngle(glm::vec3 angle)
	{
		m_Angle = angle;
		update();
	}

	void setPlanes(float zNear, float zFar) { m_Near = zNear, m_Far = zFar; }

	float FOV() const { return m_FOV; }
	float nearPlane() const { return m_Near; }
	float farPlane() const { return m_Far; }
	glm::vec3 pos() const { return m_Pos; }
	glm::vec3 angle() const { return m_Angle; }
	glm::vec3 front() const { return m_Front; }
	glm::vec3 right() const { return m_Right; }
	glm::vec3 up() const { return m_Up; }

	glm::mat4 viewMatrix()
	{
		float aX = cos(glm::radians(m_Angle.y)) * cos(glm::radians(m_Angle.x));
		float aY = cos(glm::radians(m_Angle.y)) * sin(glm::radians(m_Angle.x));
		float aZ = sin(glm::radians(m_Angle.y));
		glm::vec3 lookingAt = m_Pos + glm::vec3(aX, aY, aZ);
		glm::mat4 view = glm::lookAt(m_Pos, lookingAt, m_Up);
		return view;
	}

	glm::mat4 viewMatrix(glm::vec3 focus) const
	{
		glm::mat4 view = glm::lookAt(m_Pos, focus, m_Up);
		return view;
	}

	glm::mat4 projMatrix(int width, int height) const
	{
		glm::mat4 proj = glm::perspective(glm::radians(m_FOV), (float)width / (float)height, m_Near, m_Far);
		return proj;
	}

private:
	void update()
	{
		float aX = cos(glm::radians(m_Angle.y)) * cos(glm::radians(m_Angle.x));
		float aY = cos(glm::radians(m_Angle.y)) * sin(glm::radians(m_Angle.x));
		float aZ = sin(glm::radians(m_Angle.y));
		m_Front = glm::normalize(glm::vec3(aX, aY, aZ));
		glm::vec3 u(0.0f, 0.0f, 1.0f);
		m_Right = glm::cross(m_Front, u);
		m_Up = glm::cross(m_Right, m_Front);
	}

private:
	glm::vec3 m_Pos;
	glm::vec3 m_Angle;
	glm::vec3 m_Front;
	glm::vec3 m_Right;
	glm::vec3 m_Up;
	float m_FOV = 45.0f;
	float m_Near = 0.1f;
	float m_Far = 100.0f;
};

#endif
