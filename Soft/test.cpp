#pragma GCC optimize(3, "Ofast", "inline")
#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <thread>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "acllib.h"
#include "FrameBuffer.h"
#include "Color.h"
#include "Camera.h"
#include "Texture.h"
#include "FPSTimer.h"
#include "FrameBufferDouble.h"
#include "Hittable/HittableShapes.h"
#include "Shape/Shapes.h"
#include "Light/Lights.h"
#include "Material/Materials.h"
#include "Math/Math.h"
#include "Environment/Environments.h"
#include "Sampler/Samplers.h"
#include "Accelerator/BVH.h"

const int W_WIDTH = 640;
const int W_HEIGHT = 640;
const int MAX_THREADS = std::thread::hardware_concurrency();

bool keyPressing[256] = { false };

FrameBufferDouble<RGB24> colorBuffer(W_WIDTH, W_HEIGHT);
FrameBuffer<glm::vec3> resultBuffer(W_WIDTH, W_HEIGHT);
Camera camera({ -0.0f, -3.0f, 0.0f });
//Camera camera({ -2.0f, -5.0f, 1.0f });

bool F1Pressed = false;
bool cursorDisabled = true;
FPSTimer fpsTimer;

float aspect = (float)W_WIDTH / (float)W_HEIGHT;

std::vector<std::shared_ptr<Shape>> shapeList;
std::vector<std::shared_ptr<Light>> lightList;
std::shared_ptr<Environment> env;
float envHDRStrength = glm::exp(3.0f);

std::shared_ptr<BVH<Shape>> shapeBVH;
std::shared_ptr<BVH<Light>> lightBVH;

void printVec3(const glm::vec3 &v, std::string info = "")
{
	std::cout << info << ":  " << v.x << "  " << v.y << "  " << v.z << std::endl;
}

inline Ray getRay(const glm::vec3 &front, const glm::vec3 &right, const glm::vec3 &up, const Camera &cam, float x, float y)
{
	glm::vec3 rayDir = (front + (up * y + right * aspect * x) * (float)tan(glm::radians(cam.FOV() / 2.0))) * cam.nearPlane();

	return { cam.pos(), glm::normalize(rayDir) };
}

glm::vec3 trace(Ray ray, SurfaceInfo surfaceInfo, int depth)
{
	if (depth == 0) return env->getRadiance(ray.dir) * envHDRStrength;

	glm::vec3 hitPoint = ray.ori;
	glm::vec3 Wo = -ray.dir;
	glm::vec3 N = surfaceInfo.norm;

	glm::vec3 directRadiance(0.0f);

	for (auto &lt : lightList)
	{
		glm::vec3 randomPoint = lt->getRandomPoint();
		glm::vec3 rayDir = glm::normalize(randomPoint - hitPoint);
		float lightDist = glm::length(randomPoint - hitPoint);

		Ray lightRay(hitPoint + N * 0.0001f, rayDir);

		float tMin, tMax;
		auto occShape = shapeBVH->closestHit(lightRay, tMin, tMax);

		if (occShape != nullptr && tMin < lightDist) continue;

		glm::vec3 Wi = rayDir;
		glm::vec3 lightN = lt->surfaceNormal(randomPoint);
		glm::vec3 lightRad = lt->getRadiance(-Wi, lightN, lightDist);
		glm::vec3 outRad = surfaceInfo.material->reflectionRadiance(Wo, Wi, N, lightRad);

		directRadiance += outRad;
	}

	glm::vec3 Wi = surfaceInfo.material->getSample(hitPoint, N, Wo);
	Ray newRay(hitPoint + N * 0.0001f, Wi);

	float tmp;
	float minDistShape = 1000.0f;

	auto closestShape = shapeBVH->closestHit(newRay, minDistShape, tmp);

	float minDistLight = 1000.0f;
	glm::vec3 closestHitNorm;

	auto closestLight = lightBVH->closestHit(newRay, minDistLight, tmp);
	if (closestLight != nullptr) closestHitNorm = closestLight->surfaceNormal(newRay.get(minDistLight));

	glm::vec3 nextRadiance(0.0f);

	if (closestLight != nullptr) nextRadiance += closestLight->getRadiance(-Wi, closestHitNorm, minDistLight);

	if (closestShape == nullptr) nextRadiance += env->getRadiance(Wi) * envHDRStrength;
	else
	{
		glm::vec3 nextHitPoint = newRay.get(minDistShape);
		SurfaceInfo nextSInfo = closestShape->surfaceInfo(nextHitPoint);

		newRay.ori = nextHitPoint;
		nextRadiance += trace(newRay, nextSInfo, depth - 1);
	}

	glm::vec3 indirectRadiance = surfaceInfo.material->reflectionRadiance(Wo, Wi, N, nextRadiance);

	return directRadiance + indirectRadiance;
}

const int MAX_SPP = 2000;
int spp = 0;

void doTracing(
	FrameBuffer<glm::vec3> &fb,
	const Camera &cam,
	int startX,
	int endX)
{
	int width = fb.width;
	int height = fb.height;

	glm::vec3 u(0.0f, 0.0f, 1.0f);
	glm::vec3 f = camera.pointing();
	glm::vec3 r = glm::cross(f, u);
	u = cross(r, f);

	for (int x = startX; x < endX; x++)
	{
		for (int y = 0; y < height; y++)
		{
			float sx = 2.0f * (float)x / width - 1.0f;
			float sy = 1.0f - 2.0f * (float)y / height;

			float sx1 = 2.0f * (float)(x + 1) / width - 1.0f;
			float sy1 = 1.0f - 2.0f * (float)(y + 1) / height;

			RandomGenerator rg;
			Ray ray = getRay(f, r, u, cam, rg.get(sx, sx1), rg.get(sy, sy1));

			glm::vec3 radiance(0.0f);

			float minDistShape = 1000.0f;

			float tmp;
			auto closestShape = shapeBVH->closestHit(ray, minDistShape, tmp);

			float minDistLight = 1000.0f;
			auto closestLight = lightBVH->closestHit(ray, minDistLight, tmp);

			if (closestShape != nullptr)
			{
				if (closestLight != nullptr && minDistLight < minDistShape) radiance = closestLight->getRadiance();
				else
				{
					glm::vec3 hitPoint = ray.get(minDistShape);
					SurfaceInfo sInfo = closestShape->surfaceInfo(hitPoint);

					ray.ori = hitPoint;

					radiance = trace(ray, sInfo, 5);
				}
			}
			else if (closestLight != nullptr) radiance = closestLight->getRadiance();
			else radiance = env->getRadiance(ray.dir);

			radiance = radiance / (radiance + glm::vec3(1.0f));
			radiance = glm::pow(radiance, glm::vec3(1.0f / 2.2f));
			
			fb(x, y) = fb(x, y) * ((float)(spp) / (float)(spp + 1)) + radiance / (float)(spp + 1);
		}
	}
}

bool modified = false;

void processKey()
{
	unsigned char keyList[] = { 'W', 'S', 'A', 'D', 'Q', 'E', 'R', VK_SHIFT, VK_SPACE };

	for (int i = 0; i < 9; i++)
	{
		if (keyPressing[keyList[i]])
		{
			camera.move(keyList[i]);
			modified = true;
		}
	}
}

void render(int id)
{
	if (spp == MAX_SPP) return;
	processKey();

	std::thread threads[MAX_THREADS];

	if (modified)
	{
		colorBuffer.fill(glm::vec3(0.0f));
		spp = 0;
		modified = false;
	}

	for (int i = 0; i < MAX_THREADS; i++)
	{
		int start = (W_WIDTH / MAX_THREADS) * i;
		int end = std::min(W_WIDTH, (W_WIDTH / MAX_THREADS) * (i + 1));

		threads[i] = std::thread
		(
			doTracing,
			std::ref(resultBuffer),
			std::ref(camera),
			start, end
		);
	}

	for (auto& t : threads)
	{
		t.join();
	}

	for (int i = 0; i < W_WIDTH; i++)
	{
		for (int j = 0; j < W_HEIGHT; j++)
		{
			colorBuffer(i, j) = RGB24(resultBuffer(i, j)).toBGR24();
		}
	}

	spp++;

	std::cout << "\r" << std::setw(4) << spp << "/" << MAX_SPP << " spp  ";

	float perc = (float)spp / (float)MAX_SPP * 100.0f;
	//for (int i = 0; i < (int)perc; i++) std::cout << "#";
	
	std::cout << "  " << std::fixed << std::setprecision(2) << perc << "%";

	flushScreen((BYTE*)colorBuffer.getCurrentBuffer().bufPtr(), W_WIDTH, W_HEIGHT);
	colorBuffer.swap();
}

int lastCursorX = W_WIDTH / 2;
int lastCursorY = W_HEIGHT / 2;
bool firstCursorMove = true;

void mouse(int x, int y, int button, int event)
{
	if (cursorDisabled) return;
	modified = true;

    if (firstCursorMove)
    {
        lastCursorX = x;
        lastCursorY = y;
        firstCursorMove = false;
        return;
    }

    float offsetX = (x - lastCursorX) * CAMERA_ROTATE_SENSITIVITY;
    float offsetY = (y - lastCursorY) * CAMERA_ROTATE_SENSITIVITY;

    glm::vec3 offset(-offsetX, -offsetY, 0.0f);
    camera.rotate(offset);

    lastCursorX = x;
    lastCursorY = y;
}

void keyboard(int key, int event)
{
	if (event == KEY_DOWN)
	{
		keyPressing[key] = true;
		if (key == VK_F1) cursorDisabled ^= 1;
	}
	if (event == KEY_UP) keyPressing[key] = false;
}

int Setup()
{
	initWindow("Test", DEFAULT, DEFAULT, W_WIDTH, W_HEIGHT);

	srand(time(0));

	shapeList.push_back
	(
		std::make_shared<Quad>
		(
		 	glm::vec3(-3.0f, 0.0f, -3.0f),
			glm::vec3(3.0f, 0.0f, -3.0f),
			glm::vec3(-3.0f, 3.0f, -3.0f),
			std::make_shared<MaterialPBR>(glm::vec3(1.0f), 0.0f, 1.0f)
			//std::make_shared<SampleMaterial>(glm::vec3(1.0f), 1.0f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Quad>
		(
		 	glm::vec3(-3.0f, 0.0f, -3.0f),
			glm::vec3(-3.0f, 3.0f, -3.0f),
			glm::vec3(-3.0f, 0.0f, 3.0f),
			std::make_shared<MaterialPBR>(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 0.15f)
			//std::make_shared<SampleMaterial>(glm::vec3(1.0f, 0.0f, 0.0f), 0.15f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Quad>
		(
		 	glm::vec3(3.0f, 3.0f, -3.0f),
		 	glm::vec3(3.0f, 0.0f, -3.0f),
			glm::vec3(3.0f, 3.0f, 3.0f),
			std::make_shared<MaterialPBR>(glm::vec3(0.0f, 1.0f, 0.0f), 1.0f, 0.15f)
			//std::make_shared<SampleMaterial>(glm::vec3(0.0f, 1.0f, 0.0f), 0.15f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Quad>
		(
		 	glm::vec3(3.0f, 3.0f, 3.0f),
			glm::vec3(3.0f, 0.0f, 3.0f),
			glm::vec3(-3.0f, 3.0f, 3.0f),
			std::make_shared<MaterialPBR>(glm::vec3(1.0f), 0.0f, 1.0f)
			//std::make_shared<SampleMaterial>(glm::vec3(1.0f), 1.0f)
		)
	);

	shapeList.push_back
	(
	 	std::make_shared<Quad>
		(
		 	glm::vec3(-3.0f, 3.0f, -3.0f),
			glm::vec3(3.0f, 3.0f, -3.0f),
			glm::vec3(-3.0f, 3.0f, 3.0f),
			std::make_shared<MaterialPBR>(glm::vec3(1.0f), 1.0f, 0.15f)
			//std::make_shared<SampleMaterial>(glm::vec3(1.0f), 0.15f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(1.0f, 1.5f, -2.0f),
			1.2f,
			std::make_shared<MaterialPBR>(glm::vec3(1.0f), 1.0f, 0.015f)
			//std::make_shared<SampleMaterial>(glm::vec3(1.0f), 0.015f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(-1.2f, 1.8f, -1.0f),
			1.0f,
			std::make_shared<MaterialPBR>(glm::vec3(0.2f, 0.4f, 1.0f), 0.0f, 0.24f)
			//std::make_shared<SampleMaterial>(glm::vec3(0.2f, 0.4f, 1.0f), 0.24f)
		)
	);

	lightList.push_back
	(
		std::make_shared<QuadLight>
		(
		 	glm::vec3(-2.0f, 2.5f, 2.99f),
			glm::vec3(2.0f, 2.5f, 2.99f),
			glm::vec3(-2.0f, 0.5f, 2.99f),
			glm::vec3(20.0f)
		)
	);

	shapeBVH = std::make_shared<BVH<Shape>>(shapeList);
	auto shapeBVHInfo = shapeBVH->dfsDetailed();
	std::cout << "BVHShapes::  TreeSize: " << shapeBVH->size() << "  MaxDepth: " << shapeBVHInfo.maxDepth << "  AvgDepth: " << shapeBVHInfo.avgDepth << "\n";

	lightBVH = std::make_shared<BVH<Light>>(lightList);
	auto lightBVHInfo = lightBVH->dfsDetailed();
	std::cout << "BVHLigths::  TreeSize: " << lightBVH->size() << "  MaxDepth: " << lightBVHInfo.maxDepth << "  AvgDepth: " << lightBVHInfo.avgDepth << "\n";

	colorBuffer.fill({ 0, 0, 0 });
	resultBuffer.fill(glm::vec3(0.0f));

	camera.setFOV(90.0f);
	//camera.lookAt(glm::vec3(0.0f));
	
	env = std::make_shared<EnvSphereMapHDR>("res/texture/024.hdr");
	//env = std::make_shared<EnvSingleColor>(glm::vec3(1.0f, 0.5f, 0.5f));
	//env = std::make_shared<EnvSingleColor>(glm::vec3(0.0f));
	//env = std::make_shared<EnvTest>(glm::vec3(10.0f), 3, 2);

	registerTimerEvent(render);
	registerKeyboardEvent(keyboard);
	registerMouseEvent(mouse);

	std::cout << "Running on " << MAX_THREADS << " thread(s)\n";
	startTimer(0, 10);
	
	return 0;
}
