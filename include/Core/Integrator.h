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
	AO,
	LightPath,
	Path,
	AdjointPath,
	BDPT,
	MLT,
	PM
};

class Integrator
{
public:
	Integrator(ScenePtr scene, IntegratorType type) : mScene(scene), mType(type) {}
	virtual void renderOnePass() = 0;

	bool isFinished() const { return mFinished; }
	Film &result() { return mScene->mCamera->film(); }
	IntegratorType getType() const { return mType; }

	void setModified();
	virtual void reset() = 0;

public:
	SamplerPtr mSampler = std::make_shared<IndependentSampler>();

protected:
	IntegratorType mType;
	ScenePtr mScene;
	bool mModified = true;
	bool mFinished = false;
};

using IntegratorPtr = std::shared_ptr<Integrator>;

class PixelIndependentIntegrator : public Integrator
{
public:
	PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type);
	void renderOnePass();
	virtual Spectrum tracePixel(Ray ray, SamplerPtr sampler) = 0;

	void reset() { setModified(); }

private:
	void doTracing(int start, int end, SamplerPtr sampler);

public:
	bool mLimitSpp = false;

protected:
	const int mMaxSpp;
	int mCurspp = 0;
	int mWidth, mHeight;
};

class PathIntegrator : public PixelIndependentIntegrator
{
public:
	PathIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::Path) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

private:
	Spectrum trace(Ray ray, SurfaceInfo surf, SamplerPtr sampler);

public:
	bool mRussianRoulette = true;
	int mRRStartDepth = 3;
	int mMaxDepth = 5;
	bool mSampleLi = false;
	bool mUseMIS = true;
};

class LightPathIntegrator : public Integrator
{
public:
	LightPathIntegrator(ScenePtr scene, int pathsOnePass) :
		mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::LightPath) {}
	void renderOnePass();
	void reset();

private:
	void trace();
	void addToFilm(Vec2f uv, Spectrum val);

public:
	bool mRussianRoulette = true;
	int mMaxDepth = 5;
	float *mResultScale;

private:
	int mPathsOnePass;
	uint64_t mPathCount = 0;
	int mRRStartDepth = 3;
};

struct PathVertex;

class AdjointPathIntegrator : public PixelIndependentIntegrator
{
public:
	AdjointPathIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AdjointPath) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

private:
	std::optional<PathVertex> traceLight(SamplerPtr sampler);
	std::optional<PathVertex> traceCamera(Ray ray, SurfaceInfo surf, SamplerPtr sampler);
	Spectrum connect(const PathVertex &vLight, const PathVertex &vCamera);

	bool russianRouletteLight(float continueProb, int bounce, SamplerPtr sampler, Spectrum &throughput);
	bool russianRouletteCamera(float continueProb, int bounce, SamplerPtr sampler, Spectrum &throughput);

public:
	bool mRussianRoulette = true;
	int mRRCameraStartDepth = 3;
	int mRRLightStartDepth = 3;
	int mMaxCameraDepth = 5;
	int mMaxLightDepth = 5;
};

class AOIntegrator : public PixelIndependentIntegrator
{
public:
	AOIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AO) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

private:
	Spectrum trace(Ray ray, Vec3f N, SamplerPtr sampler);

public:
	float mRadius = 0.5f;
	int mSamplesOneTime = 1;
};

struct BDPTVertex;

class BDPTIntegrator : public PixelIndependentIntegrator
{
public:
	BDPTIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::BDPT) {};
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

private:
	std::vector<BDPTVertex> createLightPath(SamplerPtr sampler);
	std::vector<BDPTVertex> createCameraPath(const Ray &ray, SamplerPtr sampler);

	float connect(const std::vector<BDPTVertex> &lightPath, const std::vector<BDPTVertex> &cameraPath);
	Spectrum eval(const std::vector<BDPTVertex> &lightPath, const std::vector<BDPTVertex> &cameraPath);

public:
	bool mRussianRoulette = true;
	int mRRLightStartDepth = 3;
	int mRRCameraStartDepth = 3;
	int mMaxLightDepth = 5;
	int mMaxCameraDepth = 5;

private:
};