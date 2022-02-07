#pragma once

#include <iostream>
#include <optional>
#include <memory>

#include "../Utils/Buffer2D.h"
#include "Ray.h"
#include "Sampler.h"
#include "Transform.h"
#include "Spectrum.h"

const float CameraRotateSensitivity = 0.5f;
const float CameraMoveSensitivity = 0.1f;
const float CameraRollSensitivity = 0.05f;
const float CameraFOVSensitivity = 150.0f;
const float CameraPitchSensitivity = 88.0f;

using Film = Buffer2D<Spectrum>;

enum class CameraType
{
	Pinhole, ThinLens, Ortho, Panorama
};

struct CameraIiSample
{
	Vec3f Wi;
	Spectrum Ii;
	float dist;
	Vec2f uv;
	float pdf;
};

struct CameraIeSample
{
	Ray ray;
	Spectrum Ie;
	float pdfPos;
	float pdfDir;
};

class Camera
{
public:
	Camera(CameraType type) : mType(type) {}

	void move(Vec3f vect) { mPos += vect; }
	void move(int key);
	void roll(float rolAngle);
	void rotate(Vec3f rotAngle);
	
	void lookAt(Vec3f focus) { setDir(focus - mPos); }
	void setDir(Vec3f dir);
	void setPos(Vec3f p) { mPos = p; }
	void setAngle(Vec3f ang);

	Vec3f pos() const { return mPos; }
	Vec3f angle() const { return mAngle; }
	Film& film() { return mFilm; }
	CameraType type() const { return mType; }

	Vec3f f() const { return mFront; }
	Vec3f r() const { return mRight; }
	Vec3f u() const { return mUp; }

	virtual void initFilm(int w, int h) { mFilm.init(w, h); }

	virtual Vec2f rasterPos(Ray ray) = 0;

	virtual Ray generateRay(SamplerPtr sampler) = 0;
	virtual Ray generateRay(Vec2f uv, SamplerPtr sampler) = 0;

	virtual float pdfIi(Vec3f ref, Vec3f y) = 0;
	virtual std::optional<CameraIiSample> sampleIi(Vec3f ref, Vec2f u) = 0;
	virtual std::pair<float, float> pdfIe(Ray ray) = 0;
	virtual Spectrum Ie(Ray ray) = 0;

	virtual bool deltaArea() const = 0;

	static bool inFilmBound(Vec2f p);

protected:
	void update();

protected:
	CameraType mType;
	Film mFilm;

	Vec3f mPos = Vec3f(0.0f);
	Vec3f mAngle = { 90.0f, 0.0f, 0.0f };
	Vec3f mFront = { 0.0f, 1.0f, 0.0f };
	Vec3f mRight = { 1.0f, 0.0f, 0.0f };
	Vec3f mUp = { 0.0f, 0.0f, 1.0f };

	Mat3f mTBNMat;
	Mat3f mTBNInv;
};

using CameraPtr = std::shared_ptr<Camera>;

class ThinLensCamera :
	public Camera
{
public:
	ThinLensCamera(float FOV, float lensRadius = 0.0f, float focalDist = 1.0f) :
		mFOV(FOV), mLensRadius(lensRadius), mFocalDist(focalDist), mLensArea(Math::diskArea(lensRadius)),
		mIsDelta(lensRadius < 1e-6f), Camera(CameraType::ThinLens) {}

	void setFOV(float fov) { mFOV = fov; }
	void setLensRadius(float radius) { mLensRadius = radius; }
	void setFocalDist(float dist) { mFocalDist = dist; }

	float FOV() const { return mFOV; }
	float lensRadius() const { return mLensRadius; }
	float focalDist() const { return mFocalDist; }

	Vec2f rasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(Vec2f uv, SamplerPtr sampler);

	float pdfIi(Vec3f ref, Vec3f y);
	std::optional<CameraIiSample> sampleIi(Vec3f ref, Vec2f u);
	std::pair<float, float> pdfIe(Ray ray);
	Spectrum Ie(Ray ray);

	bool deltaArea() const { return mIsDelta; }

private:
	float mFOV = 45.0f;
	float mLensRadius;
	float mFocalDist;

	float mLensArea;
	bool mIsDelta;
};

class PanoramaCamera :
	public Camera
{
public:
	PanoramaCamera() : Camera(CameraType::Panorama) {}

	Vec2f rasterPos(Ray ray);

	Ray generateRay(SamplerPtr sampler);
	Ray generateRay(Vec2f uv, SamplerPtr sampler);

	float pdfIi(Vec3f ref, Vec3f y) { return 0.0f; }
	std::optional<CameraIiSample> sampleIi(Vec3f ref, Vec2f u);
	std::pair<float, float> pdfIe(Ray ray);
	Spectrum Ie(Ray ray);

	bool deltaArea() const { return true; }
};