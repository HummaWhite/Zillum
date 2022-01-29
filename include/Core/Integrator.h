#pragma once

#include <thread>
#include <mutex>

#include "../Utils/ObjReader.h"
#include "../Utils/Error.h"
#include "../Utils/Buffer2D.h"
#include "Camera.h"
#include "Texture.h"
#include "Object.h"
#include "Light.h"
#include "Material.h"
#include "Math.h"
#include "Environment.h"
#include "BVH.h"
#include "Scene.h"
#include "Sampler.h"

const int MaxThreads = std::thread::hardware_concurrency();

enum class IntegratorType
{
	AO, LightPath, Path, AdjointPath, Bidirectional, MLT, PM
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
	virtual void reset() = 0;

public:
	SamplerPtr mSampler = std::make_shared<IndependentSampler>();

protected:
	IntegratorType type;
	ScenePtr scene;
	bool modified = true;
	bool finished = false;
};

using IntegratorPtr = std::shared_ptr<Integrator>;

class PixelIndependentIntegrator :
	public Integrator
{
public:
	PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type);
	void renderOnePass();
	virtual Vec3f tracePixel(Ray ray, SamplerPtr sampler) = 0;

	void reset() { setModified(); }

private:
	void doTracing(int start, int end, SamplerPtr sampler);

public:
	bool limitSpp = false;

protected:
	const int maxSpp;
	int curSpp = 0;

	int width, height;
};

class PathIntegrator : 
	public PixelIndependentIntegrator
{
public:
	PathIntegrator(ScenePtr scene, int maxSpp) : PixelIndependentIntegrator(scene, maxSpp, IntegratorType::Path) {}
	Vec3f tracePixel(Ray ray, SamplerPtr sampler);

private:
	Vec3f trace(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler);

public:
	bool roulette = true;
	float rouletteProb = 0.6f;
	int tracingDepth = 5;
	int directLightSample = 1;
	float indirectClamp = 20.0f;
	float envStrength = 1.0f;
	bool sampleDirectLight = false;
	bool enableMIS = true;
};

class LightPathIntegrator :
    public Integrator
{
public:
    LightPathIntegrator(ScenePtr scene, int pathsOnePass) :
    	pathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::LightPath) {}
    void renderOnePass();
    void reset();

private:
    void trace();
    void addToFilm(Vec2f uv, Vec3f val);

public:
    int maxDepth = 5;
	float *resultScale;

private:
    int pathsOnePass;
	uint64_t pathCount = 0;
};

class AdjointPathIntegrator :
    public PixelIndependentIntegrator
{
public:
    AdjointPathIntegrator(ScenePtr scene, int maxSpp) :
        PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AdjointPath) {}
    Vec3f tracePixel(Ray ray, SamplerPtr sampler);

    struct Vertex
    {
        Vec3f P;
        Vec3f N;
        Vec3f Wo;
        MaterialPtr mat;
        Vec3f beta;
    };

private:
    std::optional<Vertex> findNonSpecularHit(Ray ray, SurfaceInfo sInfo, SamplerPtr sampler);
    Vec3f trace(Vertex v, SamplerPtr sampler);

public:
    int maxCameraDepth = 5;
    int maxLightDepth = 5;
};

class AOIntegrator:
	public PixelIndependentIntegrator
{
public:
	AOIntegrator(ScenePtr scene, int maxSpp):
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AO) {}
	Vec3f tracePixel(Ray ray, SamplerPtr sampler);

private:
	Vec3f trace(Ray ray, Vec3f N, SamplerPtr sampler);

public:
	float radius = 0.5f;
	int samples = 1;
};