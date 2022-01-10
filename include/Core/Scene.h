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
	Vec3f weight;
	float pdf;
};

struct IiSample
{
	Vec3f Wi;
	Vec3f weight;
	float pdf;
};

const LiSample InvalidLiSample = {Vec3f(0.0f), Vec3f(0.0f), 0.0f};
const IiSample InvalidIiSample = {Vec3f(0.0f), Vec3f(0.0f), 0.0f};

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

	float pdfSampleLight(Light *lt);
	float pdfSampleEnv();
	float powerlightAndEnv() { return lightDistrib.sum() + env->power(); }

	IiSample sampleIiCamera(Vec3f x, Vec2f u);

	void buildScene();

	std::pair<float, HittablePtr> closestHit(const Ray &ray) { return bvh->closestHit(ray); }
	bool quickIntersect(const Ray &ray, float dist) { return bvh->testIntersec(ray, dist); }

	void addHittable(HittablePtr hittable) { hittables.push_back(hittable); }
	void addLight(LightPtr light);
	void addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material);
	void addLightMesh(const char *path, TransformPtr transform, const Vec3f &power);

	bool visible(Vec3f x, Vec3f y);
	float v(Vec3f x, Vec3f y);
	float g(Vec3f x, Vec3f y, Vec3f Nx, Vec3f Ny);

public:
	std::vector<HittablePtr> hittables;
	std::vector<LightPtr> lights;
	EnvPtr env = std::make_shared<EnvSingleColor>(Vec3f(0.0f));
	CameraPtr camera;

	std::shared_ptr<BVH> bvh;
	Piecewise1D lightDistrib;
	LightSampleStrategy lightSampleStrategy = LightSampleStrategy::ByPower;
	LightSampleStrategy lightAndEnvStrategy = LightSampleStrategy::Uniform;

	AABB box;
	float boundRadius;
};

using ScenePtr = std::shared_ptr<Scene>;