#pragma once

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "Ray.h"

const float CAMERA_ROTATE_SENSITIVITY = 0.5f;
const float CAMERA_MOVE_SENSITIVITY = 0.1f;
const float CAMERA_ROLL_SENSITIVITY = 0.05f;
const float CAMERA_FOV_SENSITIVITY = 150.0f;
const float CAMERA_PITCH_LIMIT = 88.0f;

class Camera
{
public:
	Camera(glm::vec3 pos = { 0.0f, 0.0f, 0.0f }, glm::vec3 angle = { 90.0f, 0.0f, 0.0f });

	void move(glm::vec3 vect) { pos += vect; }
	void move(int key);
	void roll(float rolAngle);
	void rotate(glm::vec3 rotAngle);
	void changeFOV(float offset);
	void setFOV(float fov);
	void lookAt(glm::vec3 focus) { setDir(focus - pos); }
	void setDir(glm::vec3 dir);
	void setPos(glm::vec3 p) { pos = p; }
	void setAngle(glm::vec3 ang);
	void setAspect(float asp) { aspect = asp; }

	Ray getRay(float x, float y);
	glm::mat4 viewMatrix();
	glm::mat4 viewMatrix(glm::vec3 focus) const;

private:
	void update();

private:
	glm::vec3 pos;
	glm::vec3 angle;
	glm::vec3 front;
	glm::vec3 right;
	glm::vec3 up;
	float FOV = 45.0f;
	float aspect;
};

typedef std::shared_ptr<Camera> CameraPtr;