#pragma GCC optimize(3, "Ofast", "inline")
#include <iostream>

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
#include "Shapes.h"

const int W_WIDTH = 1280;
const int W_HEIGHT = 720;

bool keyPressing[256] = { false };

FrameBufferDouble<RGB24> colorBuffer(W_WIDTH, W_HEIGHT);
Camera camera({ 0.0f, -5.0f, 0.0f });
TextureRGB24 tex;
TextureRGB24 env;

glm::vec3 lightPos(1.0f, -2.0f, 3.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

bool F1Pressed = false;
bool cursorDisabled = false;
FPSTimer fpsTimer;

Sphere sphere(glm::vec3(0.0f), 2.0f);

float aspect = (float)W_WIDTH / (float)W_HEIGHT;

void processKey()
{
	if (keyPressing['W']) camera.move('W');
	if (keyPressing['S']) camera.move('S');
	if (keyPressing['A']) camera.move('A');
	if (keyPressing['D']) camera.move('D');
	if (keyPressing[VK_SHIFT]) camera.move(VK_SHIFT);
	if (keyPressing[VK_SPACE]) camera.move(VK_SPACE);
}

Ray getRay(Camera& cam, float x, float y)
{
	glm::vec3 u(0.0f, 0.0f, 1.0f);
	glm::vec3 p = camera.pointing();
	glm::vec3 r = glm::cross(p, u);
	u = cross(p, r);

	glm::vec3 rayDir = (p + (u + r * aspect) * (float)tan(cam.FOV() / 2.0)) * cam.nearPlane();

	return { cam.pos(), glm::normalize(rayDir) };
}

void trace(
	FrameBufferDouble<RGB24>& fb,
	Camera& cam,
	int startX,
	int endX)
{
	int width = fb.width();
	int height = fb.height();
	for (int x = startX; x < endX; x++)
	{
		for (int y = 0; y < height; y++)
		{
			float sx = 2.0f * (float)x / width - 1.0f;
			float sy = 2.0f * (float)y / height - 1.0f;

			Ray ray = getRay(cam, sx, sy);

			ClosestHit hit = sphere.closestHit(ray);

			if (hit.hit)
			{
				fb(x, y) = { 255, 255, 255 };
			}
		}
	}
}

void render(int id)
{
	colorBuffer.fill({ 255, 0, 0 });

	trace(colorBuffer, camera, 0, W_WIDTH);

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

	registerTimerEvent(render);
	registerMouseEvent(mouse);
	registerKeyboardEvent(keyboard);

	render(0);
	//startTimer(0, 10);
}
