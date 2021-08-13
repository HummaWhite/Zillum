#pragma once

#include <vector>
#include <memory>

#include "Shapes.h"
#include "Light.h"
#include "../Environment/Environments.h"
#include "../Accelerator/BVH.h"
#include "Camera.h"
#include "ObjReader.h"
#include "Object.h"

enum class LightSelectStrategy
{
	ByPower, Uniform
};

class Scene
{
public:
	Scene() = default;
	Scene(const std::vector<HittablePtr> &hittables, EnvironmentPtr environment, CameraPtr camera);
	void setupLightSampleTable();

	LightSample sampleOneLight(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2);
	LightSample sampleEnvironment(const glm::vec3 &x, const glm::vec2 &u1, const glm::vec2 &u2);
	LightSample sampleLightAndEnv(const glm::vec3 &x, const std::array<float, 5> &sample);

	float pdfSelectLight(Light *lt);
	float pdfSelectEnv(); 
	float powerlightAndEnv() { return lightDistrib.sum() + env->power(); }

	void buildScene();

	std::pair<float, HittablePtr> closestHit(const Ray &ray) { return bvh->closestHit(ray); }
	bool quickIntersect(const Ray &ray, float dist) { return bvh->testIntersec(ray, dist); }

	void addHittable(HittablePtr hittable) { hittables.push_back(hittable); }
	void addLight(LightPtr light);
	void addObjectMesh(const char *path, TransformPtr transform, MaterialPtr material);
	void addLightMesh(const char *path, TransformPtr transform, const glm::vec3 &power);

public:
	std::vector<HittablePtr> hittables;
	std::vector<LightPtr> lights;
	EnvironmentPtr env;
	CameraPtr camera;

	std::shared_ptr<BVH> bvh;
	Piecewise1D lightDistrib;
	LightSelectStrategy lightSelectStrategy = LightSelectStrategy::ByPower;
	LightSelectStrategy lightAndEnvStrategy = LightSelectStrategy::Uniform;
};

typedef std::shared_ptr<Scene> ScenePtr;