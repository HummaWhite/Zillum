#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <memory>

#include "Shapes.h"
#include "Light.h"
#include "Environments.h"
#include "../Accelerator/BVH.h"
#include "Camera.h"
#include "ObjReader.h"
#include "Object.h"

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
	glm::vec3 Wi;
	glm::vec3 weight;
	float pdf;
};

struct IiSample
{
	glm::vec3 Wi;
	glm::vec3 weight;
	float pdf;
};

const LiSample InvalidLiSample = {glm::vec3(0.0f), glm::vec3(0.0f), 0.0f};
const IiSample InvalidIiSample = {glm::vec3(0.0f), glm::vec3(0.0f), 0.0f};

class Scene
{
public:
	Scene() = default;
	Scene(const std::vector<HittablePtr> &hittables, EnvPtr environment, CameraPtr camera);
	void setupLightSampleTable();

	std::optional<LightSample> sampleOneLight(glm::vec2 u);
	LightEnvSample sampleLightAndEnv(glm::vec2 u1, float u2);

	LiSample sampleLiOneLight(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2);
	LiSample sampleLiEnv(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2);
	LiSample sampleLiLightAndEnv(const glm::vec3 &x, const std::array<float, 5> &sample);

	float pdfSampleLight(Light *lt);
	float pdfSampleEnv();
	float powerlightAndEnv() { return lightDistrib.sum() + env->power(); }

	IiSample sampleIiCamera(glm::vec3 x, glm::vec2 u);

	void buildScene();

	std::pair<float, HittablePtr> closestHit(const Ray &ray) { return bvh->closestHit(ray); }
	bool quickIntersect(const Ray &ray, float dist) { return bvh->testIntersec(ray, dist); }

	void addHittable(HittablePtr hittable) { hittables.push_back(hittable); }
	void addLight(LightPtr light);
	void addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material);
	void addLightMesh(const char *path, TransformPtr transform, const glm::vec3 &power);

	bool occlude(glm::vec3 x, glm::vec3 y);

public:
	std::vector<HittablePtr> hittables;
	std::vector<LightPtr> lights;
	EnvPtr env = std::make_shared<EnvSingleColor>(glm::vec3(0.0f));
	CameraPtr camera;

	std::shared_ptr<BVH> bvh;
	Piecewise1D lightDistrib;
	LightSampleStrategy lightSampleStrategy = LightSampleStrategy::ByPower;
	LightSampleStrategy lightAndEnvStrategy = LightSampleStrategy::Uniform;

	AABB box;
	float boundRadius;
};

typedef std::shared_ptr<Scene> ScenePtr;