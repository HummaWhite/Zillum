#pragma once

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "Ray.h"
#include "../Buffer/FrameBuffer.h"
#include "../Sampler/Sampler.h"
#include "../Math/Transform.h"

const float CAMERA_ROTATE_SENSITIVITY = 0.5f;
const float CAMERA_MOVE_SENSITIVITY = 0.1f;
const float CAMERA_ROLL_SENSITIVITY = 0.05f;
const float CAMERA_FOV_SENSITIVITY = 150.0f;
const float CAMERA_PITCH_LIMIT = 88.0f;

typedef FrameBuffer<glm::vec3> Film;

enum class CameraType
{
	ThinLens, Ortho, Panorama
};

class Camera
{
public:
	Camera(CameraType type) : type(type) {}

	void move(glm::vec3 vect) { pos += vect; }
	void move(int key);
	void roll(float rolAngle);
	void rotate(glm::vec3 rotAngle);
	
	void lookAt(glm::vec3 focus) { setDir(focus - pos); }
	void setDir(glm::vec3 dir);
	void setPos(glm::vec3 p) { pos = p; }
	void setAngle(glm::vec3 ang);

	glm::vec3 getPos() const { return pos; }
	glm::vec3 getAngle() const { return angle; }
	Film& getFilm() { return film; }
	CameraType getType() const { return type; }

	void initFilm(int w, int h) { film.init(w, h); }

	virtual Ray generateRay(SamplerPtr sampler) = 0;
	virtual Ray generateRay(glm::vec2 uv, SamplerPtr sampler) = 0;

protected:
	void update();

protected:
	CameraType type;
	Film film;

	glm::vec3 pos = glm::vec3(0.0f);
	glm::vec3 angle = { 90.0f, 0.0f, 0.0f };
	glm::vec3 front = { 0.0f, 1.0f, 0.0f };
	glm::vec3 right = { 1.0f, 0.0f, 0.0f };
	glm::vec3 up = { 0.0f, 0.0f, 1.0f };
};

typedef std::shared_ptr<Camera> CameraPtr;

class ThinLensCamera :
	public Camera
{
public:
	ThinLensCamera(float FOV, float lensRadius = 0.0f, float focalDist = 1.0f) :
		FOV(FOV), lensRadius(lensRadius), focalDist(focalDist), Camera(CameraType::ThinLens) {}

	void setFOV(float fov) { FOV = fov; }
	void setLensRadius(float radius) { lensRadius = radius; }
	void setFocalDist(float dist) { focalDist = dist; }

	float getFOV() const { return FOV; }
	float getLensRadius() const { return lensRadius; }
	float getFocalDist() const { return focalDist; }

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(glm::vec2 uv, SamplerPtr sampler);

private:
	float FOV = 45.0f;
	float lensRadius;
	float focalDist;
};

class PanoramaCamera :
	public Camera
{
public:
	PanoramaCamera() : Camera(CameraType::Panorama) {}

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(glm::vec2 uv, SamplerPtr sampler);
};