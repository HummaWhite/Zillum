#pragma once

#include <iostream>
#include <memory>

#include "Ray.h"
#include "../Buffer/Buffer2D.h"
#include "../Sampler/Sampler.h"
#include "../Math/Transform.h"

const float CameraRotateSensitivity = 0.5f;
const float CameraMoveSensitivity = 0.1f;
const float CameraRollSensitivity = 0.05f;
const float CameraFOVSensitivity = 150.0f;
const float CameraPitchSensitivity = 88.0f;

using Film = Buffer2D<Vec3f>;

enum class CameraType
{
	ThinLens, Ortho, Panorama
};

struct CameraIiSample
{
	Vec3f Wi;
	Vec3f Ii;
	float dist;
	Vec2f uv;
	float pdf;
};

struct CameraIeSample
{
	Ray ray;
	Vec3f Ie;
	float pdfPos;
	float pdfDir;
};

const CameraIiSample InvalidCamIiSample = { Vec3f(), Vec3f(), 0.0f, Vec2f(), 0.0f };

class Camera
{
public:
	Camera(CameraType type) : type(type) {}

	void move(Vec3f vect) { pos += vect; }
	void move(int key);
	void roll(float rolAngle);
	void rotate(Vec3f rotAngle);
	
	void lookAt(Vec3f focus) { setDir(focus - pos); }
	void setDir(Vec3f dir);
	void setPos(Vec3f p) { pos = p; }
	void setAngle(Vec3f ang);

	Vec3f getPos() const { return pos; }
	Vec3f getAngle() const { return angle; }
	Film& getFilm() { return film; }
	CameraType getType() const { return type; }

	Vec3f f() const { return front; }
	Vec3f r() const { return right; }
	Vec3f u() const { return up; }

	void initFilm(int w, int h) { film.init(w, h); }

	virtual Vec2f getRasterPos(Ray ray) = 0;

	virtual Ray generateRay(SamplerPtr sampler) = 0;
	virtual Ray generateRay(Vec2f uv, SamplerPtr sampler) = 0;

	virtual float pdfIi(Vec3f ref, Vec3f y) = 0;
	virtual CameraIiSample sampleIi(Vec3f ref, Vec2f u) = 0;
	// [pdfPos, pdfDir]
	virtual std::pair<float, float> pdfIe(Ray ray) = 0;
	// This is really tough to implement, fortunately it's not likely to be used
	virtual Vec3f Ie(Ray ray) = 0;

	virtual bool deltaArea() const = 0;

	static bool inFilmBound(Vec2f p);

protected:
	void update();

protected:
	CameraType type;
	Film film;

	Vec3f pos = Vec3f(0.0f);
	Vec3f angle = { 90.0f, 0.0f, 0.0f };
	Vec3f front = { 0.0f, 1.0f, 0.0f };
	Vec3f right = { 1.0f, 0.0f, 0.0f };
	Vec3f up = { 0.0f, 0.0f, 1.0f };

	Mat3f tbnMat;
	Mat3f tbnInv;
};

using CameraPtr = std::shared_ptr<Camera>;

class ThinLensCamera :
	public Camera
{
public:
	ThinLensCamera(float FOV, float lensRadius = 0.0f, float focalDist = 1.0f) :
		FOV_(FOV), lensRadius(lensRadius), focalDist(focalDist), lensArea(Math::diskArea(lensRadius)),
		approxPinhole(lensRadius < 1e-6f), Camera(CameraType::ThinLens) {}

	void setFOV(float fov) { FOV_ = fov; }
	void setLensRadius(float radius) { lensRadius = radius; }
	void setFocalDist(float dist) { focalDist = dist; }

	float getFOV() const { return FOV_; }
	float getLensRadius() const { return lensRadius; }
	float getFocalDist() const { return focalDist; }

	Vec2f getRasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(Vec2f uv, SamplerPtr sampler);

	float pdfIi(Vec3f ref, Vec3f y);
	CameraIiSample sampleIi(Vec3f ref, Vec2f u);
	std::pair<float, float> pdfIe(Ray ray);
	Vec3f Ie(Ray ray);

	bool deltaArea() const { return approxPinhole; }

private:
	float FOV_ = 45.0f;
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

	Vec2f getRasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(Vec2f uv, SamplerPtr sampler);

	float pdfIi(Vec3f ref, Vec3f y) { return 0.0f; }
	CameraIiSample sampleIi(Vec3f ref, Vec2f u);
	std::pair<float, float> pdfIe(Ray ray);
	Vec3f Ie(Ray ray);

	bool deltaArea() const { return true; }
};