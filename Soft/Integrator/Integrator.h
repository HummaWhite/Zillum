#pragma once

#include <thread>
#include <mutex>

#include "../Buffer/FrameBuffer.h"
#include "../Display/Color.h"
#include "../Scene/Camera.h"
#include "../Buffer/Texture.h"
#include "../Scene/Object.h"
#include "../Scene/Light.h"
#include "../Material/Materials.h"
#include "../Math/Math.h"
#include "../Scene/Environments.h"
#include "../Accelerator/BVH.h"
#include "../Scene/ObjReader.h"
#include "../Scene/Scene.h"
#include "../Sampler/Samplers.h"

const int MaxThreads = std::thread::hardware_concurrency();

enum class IntegratorType
{
	AO, LightPath, Path, Bidirectional, MLT, PM
};

class Integrator
{
public:
	Integrator(ScenePtr scene, IntegratorType type) :
		scene(scene), type(type) {}
	virtual void renderOnePass() = 0;

	bool isFinished() const { return finished; }
	Film& result() { return scene->camera->getFilm(); }
	IntegratorType getType() const { return type; }

	void setModified();

public:
	SamplerPtr mSampler = std::make_shared<IndependentSampler>();

protected:
	IntegratorType type;
	ScenePtr scene;
	bool modified = true;
	bool finished = false;
};

typedef std::shared_ptr<Integrator> IntegratorPtr;

class PixelIndependentIntegrator :
	public Integrator
{
public:
	PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type);
	void renderOnePass();
	virtual glm::vec3 tracePixel(Ray ray, SamplerPtr sampler) = 0;

private:
	void doTracing(int start, int end, SamplerPtr sampler);

public:
	bool limitSpp = false;

protected:
	const int maxSpp;
	int curSpp = 0;

	int width, height;
};