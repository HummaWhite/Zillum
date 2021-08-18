#pragma once

#include <iostream>
#include <memory>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../glm/gtc/type_ptr.hpp"

#include "Ray.h"
#include "../Buffer/Buffer2D.h"
#include "../Sampler/Sampler.h"
#include "../Math/Transform.h"

const float CAMERA_ROTATE_SENSITIVITY = 0.5f;
const float CAMERA_MOVE_SENSITIVITY = 0.1f;
const float CAMERA_ROLL_SENSITIVITY = 0.05f;
const float CAMERA_FOV_SENSITIVITY = 150.0f;
const float CAMERA_PITCH_LIMIT = 88.0f;

typedef Buffer2D<glm::vec3> Film;

enum class CameraType
{
	ThinLens, Ortho, Panorama
};

struct CameraIiSample
{
	glm::vec3 Wi;
	glm::vec3 Ii;
	float dist;
	glm::vec2 uv;
	float pdf;
};

struct CameraIeSample
{
	Ray ray;
	glm::vec3 Ie;
	float pdfPos;
	float pdfDir;
};

const CameraIiSample InvalidCamIiSample = { glm::vec3(), glm::vec3(), 0.0f, glm::vec2(), 0.0f };

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

	glm::vec3 f() const { return front; }
	glm::vec3 r() const { return right; }
	glm::vec3 u() const { return up; }

	void initFilm(int w, int h) { film.init(w, h); }

	virtual glm::vec2 getRasterPos(Ray ray) = 0;

	virtual Ray generateRay(SamplerPtr sampler) = 0;
	virtual Ray generateRay(glm::vec2 uv, SamplerPtr sampler) = 0;

	virtual float pdfIi(glm::vec3 x, glm::vec3 y) = 0;
	virtual CameraIiSample sampleIi(glm::vec3 x, glm::vec2 u) = 0;
	// [pdfPos, pdfDir]
	virtual std::pair<float, float> pdfIe(Ray ray) = 0;
	// This is really tough to implement, fortunately it's not likely to be used
	virtual glm::vec3 Ie(Ray ray) = 0;

	virtual bool deltaArea() const = 0;

	static bool inFilmBound(glm::vec2 p);

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

	glm::mat3 tbnMat;
	glm::mat3 tbnInv;
};

typedef std::shared_ptr<Camera> CameraPtr;

class ThinLensCamera :
	public Camera
{
public:
	ThinLensCamera(float FOV, float lensRadius = 0.0f, float focalDist = 1.0f) :
		FOV(FOV), lensRadius(lensRadius), focalDist(focalDist), lensArea(Math::diskArea(lensRadius)),
		approxPinhole(lensRadius < 1e-6f), Camera(CameraType::ThinLens) {}

	void setFOV(float fov) { FOV = fov; }
	void setLensRadius(float radius) { lensRadius = radius; }
	void setFocalDist(float dist) { focalDist = dist; }

	float getFOV() const { return FOV; }
	float getLensRadius() const { return lensRadius; }
	float getFocalDist() const { return focalDist; }

	glm::vec2 getRasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(glm::vec2 uv, SamplerPtr sampler);

	float pdfIi(glm::vec3 x, glm::vec3 y);
	CameraIiSample sampleIi(glm::vec3 x, glm::vec2 u);
	std::pair<float, float> pdfIe(Ray ray);
	glm::vec3 Ie(Ray ray);

	bool deltaArea() const { return approxPinhole; }

private:
	float FOV = 45.0f;
	float lensRadius;
	float focalDist;

	float lensArea;
	bool approxPinhole;
};

class PanoramaCamera :
	public Camera
{
public:
	PanoramaCamera() : Camera(CameraType::Panorama) {}

	glm::vec2 getRasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(glm::vec2 uv, SamplerPtr sampler);

	float pdfIi(glm::vec3 x, glm::vec3 y) { return 0.0f; }
	CameraIiSample sampleIi(glm::vec3 x, glm::vec2 u);
	std::pair<float, float> pdfIe(Ray ray);
	glm::vec3 Ie(Ray ray);

	bool deltaArea() const { return true; }
};