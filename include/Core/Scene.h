#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <memory>

#include "../Utils/ObjReader.h"
#include "Light.h"
#include "Object.h"
#include "Environment.h"
#include "Camera.h"
#include "Shape.h"
#include "BVH.h"

enum class LightSampleStrategy
{
	ByPower, Uniform
};

struct LightSample
{
	LightPtr lt;
	float pdf;
};

struct LightEnvSample
{
	std::variant<LightPtr, EnvPtr> sample;
	float pdf;
};

struct LiSample
{
	Vec3f Wi;
	Spectrum weight;
	float pdf;
};

struct LeSample
{
	Ray emiRay;
	Spectrum weight;
	float pdf;
};

struct IiSample
{
	Vec3f Wi;
	Vec3f weight;
	float pdf;
};

const LiSample InvalidLiSample = { Vec3f(0.0f), Spectrum(0.0f), 0.0f };
const IiSample InvalidIiSample = { Vec3f(0.0f), Spectrum(0.0f), 0.0f };

class Scene
{
public:
	Scene() = default;
	Scene(const std::vector<HittablePtr> &hittables, EnvPtr environment, CameraPtr camera);
	void setupLightSampleTable();

	std::optional<LightSample> sampleOneLight(Vec2f u);
	LightEnvSample sampleLightAndEnv(Vec2f u1, float u2);

	LiSample sampleLiOneLight(const Vec3f &x, const Vec2f &u1, const Vec2f &u2);
	LiSample sampleLiEnv(const Vec3f &x, const Vec2f &u1, const Vec2f &u2);
	LiSample sampleLiLightAndEnv(const Vec3f &x, const std::array<float, 5> &sample);

	LeSample sampleLeOneLight(const std::array<float, 6> &sample);
	LeSample sampleLeEnv(const std::array<float, 6> &sample);
	LeSample sampleLeLightAndEnv(const std::array<float, 7> &sample);

	float pdfSampleLight(Light *lt);
	float pdfSampleEnv();
	float powerlightAndEnv() { return mLightDistrib.sum() + mEnv->power(); }

	IiSample sampleIiCamera(Vec3f x, Vec2f u);

	void buildScene();

	std::pair<float, HittablePtr> closestHit(const Ray &ray) { return mBvh->closestHit(ray); }
	bool quickIntersect(const Ray &ray, float dist) { return mBvh->testIntersec(ray, dist); }

	void addHittable(HittablePtr hittable) { mHittables.push_back(hittable); }
	void addLight(LightPtr light);
	void addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material);
	void addLightMesh(const char *path, TransformPtr transform, const Spectrum &power);

	bool visible(Vec3f x, Vec3f y);
	float v(Vec3f x, Vec3f y);
	float g(Vec3f x, Vec3f y, Vec3f Nx, Vec3f Ny);

public:
	std::vector<HittablePtr> mHittables;
	std::vector<LightPtr> mLights;
	EnvPtr mEnv = std::make_shared<EnvSingleColor>(Spectrum(0.0f));
	CameraPtr mCamera;

	std::shared_ptr<BVH> mBvh;
	Piecewise1D mLightDistrib;
	LightSampleStrategy mLightSampleStrategy = LightSampleStrategy::ByPower;
	LightSampleStrategy mLightAndEnvStrategy = LightSampleStrategy::Uniform;

	AABB mBound;
	float mBoundRadius;
};

using ScenePtr = std::shared_ptr<Scene>;