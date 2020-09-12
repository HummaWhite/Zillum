#pragma GCC optimize(3, "Ofast", "inline")
#include <iostream>
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
#include "ObjReader.h"
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

const int W_WIDTH = 1280;
const int W_HEIGHT = 720;
const int MAX_THREADS = std::thread::hardware_concurrency();

bool keyPressing[256] = { false };

FrameBufferDouble<RGB24> colorBuffer(W_WIDTH, W_HEIGHT);
FrameBuffer<glm::vec3> resultBuffer(W_WIDTH, W_HEIGHT);
Camera camera({ 2.0f, -3.0f, 0.0f });

bool F1Pressed = false;
bool cursorDisabled = false;
FPSTimer fpsTimer;

float aspect = (float)W_WIDTH / (float)W_HEIGHT;

std::vector<std::shared_ptr<Shape>> shapeList;
std::vector<std::shared_ptr<Light>> lightList;
std::shared_ptr<Environment> env;

void printVec3(const glm::vec3 &v, std::string info = "")
{
	std::cout << info << ":  " << v.x << "  " << v.y << "  " << v.z << std::endl;
}

void processKey()
{
	if (keyPressing['W']) camera.move('W');
	if (keyPressing['S']) camera.move('S');
	if (keyPressing['A']) camera.move('A');
	if (keyPressing['D']) camera.move('D');
	if (keyPressing[VK_SHIFT]) camera.move(VK_SHIFT);
	if (keyPressing[VK_SPACE]) camera.move(VK_SPACE);
}

inline Ray getRay(const glm::vec3 &front, const glm::vec3 &right, const glm::vec3 &up, const Camera &cam, float x, float y)
{
	glm::vec3 rayDir = (front + (up * y + right * aspect * x) * (float)tan(glm::radians(cam.FOV() / 2.0))) * cam.nearPlane();

	return { cam.pos(), glm::normalize(rayDir) };
}

//glm::vec3 atmosphereColor(140.0f, 140.0f, 1000.0f);

glm::vec3 trace(Ray ray, int depth)
{
	glm::vec3 atmosphereColor = env->getRadiance(ray.dir);

	if (depth == 0) return atmosphereColor;

	float minDistShape = 1000.0f;
	std::shared_ptr<Shape> closestShape;

	for (auto &sp : shapeList)
	{
		HitInfo hInfo = sp->closestHit(ray);

		if (hInfo.hit && hInfo.dist < minDistShape)
		{
			minDistShape = hInfo.dist;
			closestShape = sp;
		}
	}

	float minDistLight = 1000.0f;
	std::shared_ptr<Light> closestLight;

	for (auto &sp : lightList)
	{
		HitInfo hInfo = sp->closestHit(ray);

		if (hInfo.hit && hInfo.dist < minDistLight)
		{
			minDistLight = hInfo.dist;
			closestLight = sp;
		}
	}

	if (closestLight != nullptr && minDistLight < minDistShape) return closestLight->getRadiance();

	if (closestShape == nullptr) return atmosphereColor;

	glm::vec3 hitPoint = ray.ori + ray.dir * minDistShape;
	SurfaceInfo sInfo = closestShape->surfaceInfo(hitPoint);

	glm::vec3 dir = sInfo.material->getSample(hitPoint, sInfo.norm, -ray.dir);
	Ray nextTrace(hitPoint, dir);

	glm::vec3 directRadiance(0.0f);

	for (auto &lt : lightList)
	{
		glm::vec3 randomPoint = lt->getRandomPoint();

		Ray lightRay(hitPoint, glm::normalize(randomPoint - hitPoint));
		bool occlusion = false;
		
		for (auto &sp : shapeList)
		{
			HitInfo info = sp->closestHit(lightRay);
			if (info.hit)
			{
				occlusion = true;
				break;
			}
		}

		if (occlusion) continue;

		float dist = glm::length(randomPoint - hitPoint);

		glm::vec3 lightRad = lt->getRadiance() / (dist * dist);
		glm::vec3 outRad = sInfo.material->reflectionRadiance(-ray.dir, -lightRay.dir, sInfo.norm, lightRad);

		directRadiance += outRad;
	}

	glm::vec3 indirectRadiance = sInfo.material->reflectionRadiance(-ray.dir, -nextTrace.dir, sInfo.norm, trace(nextTrace, depth - 1));

	return directRadiance + indirectRadiance * glm::pi<float>();
}

const int MAX_SPP = 1000;
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
			
			glm::vec3 radiance = trace(ray, 6);
			radiance = radiance / (radiance + glm::vec3(1.0f));
			radiance = glm::pow(radiance, glm::vec3(1.0f / 2.2f));
			
			fb(x, y) = fb(x, y) * ((float)(spp) / (float)(spp + 1)) + radiance / (float)(spp + 1);
		}
	}
}

void render(int id)
{
	if (spp == MAX_SPP) return;

	std::thread threads[MAX_THREADS];

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

	std::cout << "#";
	spp++;

	flushScreen((BYTE*)colorBuffer.getCurrentBuffer().bufPtr(), W_WIDTH, W_HEIGHT);
	colorBuffer.swap();
}

int lastCursorX = W_WIDTH / 2;
int lastCursorY = W_HEIGHT / 2;
bool firstCursorMove = true;

void mouse(int x, int y, int button, int event)
{
	if (cursorDisabled) return;

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
		std::make_shared<Sphere>
		(
			glm::vec3(0.0f, 0.0f, -0.9f),
			1.0f,
			std::make_shared<MaterialPBR>(glm::vec3(0.2f, 0.6f, 0.9f), 0.8f, 1.0f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(3.5f, -0.5f, -1.5f),
			0.5f,
			std::make_shared<MaterialPBR>(glm::vec3(0.7f, 0.6f, 0.5f), 0.0f, 0.1f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(3.0f, 1.5f, 0.0f),
			2.0f,
			std::make_shared<MaterialPBR>(glm::vec3(0.9f, 0.05f, 0.05f), 0.85f, 0.024f)
		)
	);

	shapeList.push_back
	(
		std::make_shared<Triangle>
		(
			glm::vec3(-100.0f, -10.0f, -2.0f),
			glm::vec3(100.0f, -10.0f, -2.0f),
			glm::vec3(0.0f, 100.0f, -2.0f),
			std::make_shared<MaterialPBR>(glm::vec3(1.0f), 0.2f, 0.024f)
		)
	);

	lightList.push_back
	(
		std::make_shared<TriangleLight>
		(
			glm::vec3(0.0f, -4.0f, 2.0f),
			glm::vec3(4.5f, 4.0f, 2.0f),
			glm::vec3(4.5f, -4.0f, 2.0f),
			glm::vec3(50.0f)
		)
	);

	/*lightList.push_back
	(
		std::make_shared<SphereLight>
		(
			glm::vec3(2.5f, -1.0f, 1.5f),
			0.1f,
			glm::vec3(10.0f)
		)
	);*/

	colorBuffer.fill({ 0, 0, 0 });
	resultBuffer.fill(glm::vec3(0.0f));

	camera.setFOV(90.0f);
	
	env = std::make_shared<EnvSphereMapHDR>("res/texture/090.hdr");
	//env = std::make_shared<EnvSingleColor>(glm::vec3(1.0f, 0.5f, 0.5f));

	registerTimerEvent(render);
	registerKeyboardEvent(keyboard);

	startTimer(0, 10);
}
