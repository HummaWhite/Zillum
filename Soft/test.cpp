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

const int W_WIDTH = 1280;
const int W_HEIGHT = 720;
const int MAX_THREADS = std::thread::hardware_concurrency();

bool keyPressing[256] = { false };

FrameBufferDouble<RGB24> colorBuffer(W_WIDTH, W_HEIGHT);
Camera camera({ 0.0f, -5.0f, 0.0f });
TextureRGB24 tex;
TextureRGB24 env;

glm::vec3 lightPos(1.0f, -2.0f, 3.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
glm::vec3 atmosphereColor(0.1f);

bool F1Pressed = false;
bool cursorDisabled = false;
FPSTimer fpsTimer;

float aspect = (float)W_WIDTH / (float)W_HEIGHT;

std::vector<std::shared_ptr<Shape>> shapeList;
std::vector<std::shared_ptr<Light>> lightList;

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

glm::vec3 trace(Ray ray, int depth)
{
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

	for (auto &lt : lightList)
	{
		HitInfo hInfo = lt->closestHit(ray);

		if (hInfo.hit && hInfo.dist < minDistLight)
		{
			minDistLight = hInfo.dist;
			closestLight = lt;
		}
	}

	if (closestShape == nullptr && closestLight == nullptr) return atmosphereColor;

	bool lightHit = minDistLight < minDistShape;

	if (lightHit)
	{
		return closestLight->getRadiance();
	}

	glm::vec3 hitPoint = ray.ori + ray.dir * minDistShape;
	SurfaceInfo sInfo = closestShape->surfaceInfo(hitPoint);

	Ray rayIn = { hitPoint, ray.dir };
	Ray scatterRay = sInfo.material->scatter(rayIn, sInfo.norm);

	glm::vec3 radiance = trace(scatterRay, depth - 1);

	return radiance * sInfo.material->reflectionRate();
}

void doTracing(
	FrameBufferDouble<RGB24> &fb,
	const Camera &cam,
	int startX,
	int endX)
{
	int width = fb.width();
	int height = fb.height();

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

			Ray ray = getRay(f, r, u, cam, sx, sy);

			glm::vec3 color = trace(ray, 3);

			fb(x, y) = RGB24(color);
		}
	}
}

void render(int id)
{
	colorBuffer.fill({ 0, 0, 0 });

	if (!cursorDisabled) processKey();
	//fpsTimer.work();

	std::thread threads[MAX_THREADS];

	for (int i = 0; i < MAX_THREADS; i++)
	{
		int start = (W_WIDTH / MAX_THREADS) * i;
		int end = std::min(W_WIDTH, (W_WIDTH / MAX_THREADS) * (i + 1));

		threads[i] = std::thread
		(
		 	doTracing,
			std::ref(colorBuffer),
			std::ref(camera),
			start, end
		);
	}

	for (auto& t : threads)
	{
		t.join();
	}

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

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(0.0f),
			1.0f,
			std::make_shared<SampleMaterial>(glm::vec3(1.0f, 0.6f, 0.2f))
		)
	);

	shapeList.push_back
	(
		std::make_shared<Sphere>
		(
			glm::vec3(2.0f),
			0.5f,
			std::make_shared<SampleMaterial>(glm::vec3(0.5f, 0.6f, 0.7f))
		)
	);

	shapeList.push_back
	(
		std::make_shared<Triangle>
		(
			glm::vec3(-4.0f, -2.0f, -2.0f),
			glm::vec3(4.0f, -2.0f, -2.0f),
			glm::vec3(0.0f, 6.0f, -2.0f),
			std::make_shared<SampleMaterial>(glm::vec3(0.8f, 0.7f, 0.6f))
		)
	);

	lightList.push_back
	(
		std::make_shared<SphereLight>
		(
			glm::vec3(0.0f, -1.0f, 4.0f),
			0.5f,
			glm::vec3(0.5f, 0.6f, 0.7f)
		)
	);

	camera.setFOV(90.0f);

	registerTimerEvent(render);
	registerMouseEvent(mouse);
	registerKeyboardEvent(keyboard);

	//render(0);
	startTimer(0, 10);
}
