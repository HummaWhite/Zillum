#pragma once

#include <windows.h>
#include <iostream>
#include <olectl.h>
#include <iomanip>
#include <vector>
#include <memory>
#include <thread>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Buffer/Texture.h"
#include "Buffer/FrameBufferDouble.h"
#include "Integrator/Path.h"
#include "Integrator/AmbientOcclusion.h"
#include "Integrator/LightPath.h"
#include "Integrator/AdjointPath.h"

class Application
{
public:
	void init(const std::string& name, HINSTANCE instance, int width, int height, int spp = 0);
	LRESULT process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void render();

private:
	void initScene(int spp);
	void writeBuffer();
	void flushScreen();
	void processKey();
	void saveImage();

private:
	std::string name;
	int windowWidth;
	int windowHeight;

	HINSTANCE instance;
	HWND window;
	HDC memdc;
	HBITMAP bitmap;

	bool keyPressing[256];
	bool F1Pressed = false;
	bool cursorDisabled = true;
	int lastCursorX;
	int lastCursorY;
	bool firstCursorMove = true;

	int toneMappingMethod = 2;

	FrameBufferDouble<RGB24> colorBuffer;
	IntegratorPtr integrator;
	ScenePtr scene;
};
