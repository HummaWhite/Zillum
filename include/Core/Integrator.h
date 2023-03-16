#pragma once

#include <thread>
#include <mutex>

#include "Utils/ObjReader.h"
#include "Utils/Error.h"
#include "Utils/Buffer2D.h"
#include "Utils/Timer.h"
#include "Camera.h"
#include "Texture.h"
#include "Object.h"
#include "Light.h"
#include "BSDF.h"
#include "Math.h"
#include "Environment.h"
#include "BVH.h"
#include "Scene.h"
#include "Sampler.h"

const int MaxThreads = std::thread::hardware_concurrency();
const int TracingDepthLimit = 64;

enum class IntegratorType {
	AO,
	LightPath,
	Path,
	TPT,
	BDPT,
	PSSMLT,
	PM,
	PPM,
	SPPM,
	VCM
};

class Integrator {
public:
	Integrator(ScenePtr scene, IntegratorType type) : mScene(scene), mType(type) {}
	virtual void renderOnePass() = 0;

	bool isFinished() const { return mFinished; }
	Film &result() { return mScene->mCamera->film(); }
	IntegratorType getType() const { return mType; }

	void setModified();
	virtual void reset() = 0;
	virtual void addToFilmLocked(const Vec2f &uv, const Spectrum &val);
	void addToDebugBuffer(int index, const Vec2f &uv, const Spectrum &val);
	void addToDebugBuffer(int index, const Vec2i &pixel, const Spectrum &val);

public:
	SamplerPtr mSampler;
	float mResultScale = 1.0f;
	int mThreads = 1;

	std::vector<Film> mDebugBuffers;
	std::vector<Buffer2D<std::mutex>> mDebugBufLockers;

protected:
	IntegratorType mType;
	ScenePtr mScene;
	bool mModified = true;
	bool mFinished = false;
};

using IntegratorPtr = std::shared_ptr<Integrator>;

class PixelIndependentIntegrator : public Integrator {
public:
	PixelIndependentIntegrator(ScenePtr scene, int maxSpp, IntegratorType type);
	void renderOnePass();
	virtual Spectrum tracePixel(Ray ray, SamplerPtr sampler) = 0;
	virtual void scaleResult();

	void reset() { setModified(); }

private:
	void doTracing(int start, int end, SamplerPtr sampler);

public:
	bool mLimitSpp = false;

protected:
	const int mMaxSpp;
	int mCurspp = 0;
	int mWidth, mHeight;
	Vec2i mPixelPos;
};

struct PathIntegParam {
	bool russianRoulette = true;
	int rrStartDepth = 3;
	int maxDepth = 5;
	bool sampleDirect = true;
	bool MIS = true;
	float spp = 0;
};

class PathIntegrator : public PixelIndependentIntegrator {
public:
	PathIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::Path) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

public:
	PathIntegParam mParam;
};

class PathIntegrator2 : public Integrator {
public:
	PathIntegrator2(ScenePtr scene, int maxSpp, int pathsOnePass) :
		mMaxSpp(maxSpp), mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::Path) {}
	void renderOnePass();
	void reset();

private:
	void trace(int paths, SamplerPtr sampler);

public:
	PathIntegParam mParam;

private:
	int mMaxSpp;
	int mPathsOnePass;
};

struct LightPathIntegParam {
	bool russianRoulette = true;
	int maxDepth = 5;
};

class LightPathIntegrator : public Integrator {
public:
	LightPathIntegrator(ScenePtr scene, int pathsOnePass) :
		mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::LightPath) {}
	void renderOnePass();
	void reset();

private:
	void trace(SamplerPtr sampler);
	void traceOnePath(SamplerPtr sampler);

public:
	LightPathIntegParam mParam;

private:
	int mPathsOnePass;
	uint64_t mPathCount = 0;
	int mRRStartDepth = 3;
};

struct BDPTIntegParam {
	bool rrLightPath = true;
	bool rrCameraPath = true;
	int rrLightStartDepth = 3;
	int rrCameraStartDepth = 3;
	int maxLightDepth = 5;
	int maxCameraDepth = 5;
	int maxConnectDepth = TracingDepthLimit;
	bool resampleEndPoint = true;
	bool stochasticConnect = false;
	bool debug = false;
	Vec2i debugStrategy;
	float spp = 0;
};

struct Path;

class BDPTIntegrator : public PixelIndependentIntegrator {
public:
	BDPTIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::BDPT) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);
	void scaleResult() override;

	void initDebugBuffers(int width, int height);

private:
	Spectrum eval(Path &lightPath, Path &cameraPath, SamplerPtr sampler);

public:
	BDPTIntegParam mParam;
	SamplerPtr mLightSampler;
};

class BDPTIntegrator2 : public Integrator {
public:
	BDPTIntegrator2(ScenePtr scene, int maxSpp, int pathsOnePass) :
		mMaxSpp(maxSpp), mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::BDPT) {}
	void renderOnePass();
	void reset();

private:
	void trace(int paths, SamplerPtr lightSampler, SamplerPtr cameraSampler);
	void traceOnePath(SamplerPtr lightSampler, SamplerPtr cameraSampler);

public:
	BDPTIntegParam mParam;
	SamplerPtr mLightSampler;

private:
	int mMaxSpp;
	int mPathsOnePass;
};

struct TriplePathIntegParam {
	bool rrLightPath = true;
	bool rrCameraPath = true;
	int rrLightStartDepth = 3;
	int rrCameraStartDepth = 3;
	int maxLightDepth = 5;
	int maxCameraDepth = 5;
	float spp = 0;
};

class TriplePathIntegrator: public Integrator {
public:
	TriplePathIntegrator(ScenePtr scene, int maxSpp, int pathsOnePass) :
		mMaxSpp(maxSpp), mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::TPT) {}
	void renderOnePass();
	void reset();

private:
	void trace(int paths, SamplerPtr sampler);
	void traceLightPath(SamplerPtr sampler);

public:
	TriplePathIntegParam mParam;

private:
	int mMaxSpp;
	int mPathsOnePass;
};

struct AOIntegParam {
	float radius = 0.5f;
	int samplesOneTime = 1;
	float spp = 0;
};

class AOIntegrator : public PixelIndependentIntegrator {
public:
	AOIntegrator(ScenePtr scene, int maxSpp) :
		PixelIndependentIntegrator(scene, maxSpp, IntegratorType::AO) {}
	Spectrum tracePixel(Ray ray, SamplerPtr sampler);

public:
	AOIntegParam mParam;
};

class AOIntegrator2 : public Integrator {
public:
	AOIntegrator2(ScenePtr scene, int maxSpp, int pathsOnePass) :
		mMaxSpp(maxSpp), mPathsOnePass(pathsOnePass), Integrator(scene, IntegratorType::AO) {}
	void renderOnePass();
	void reset();

private:
	void trace(int paths, SamplerPtr sampler);

public:
	AOIntegParam mParam;

private:
	int mMaxSpp;
	int mPathsOnePass;
};