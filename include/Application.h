#pragma once

#include <windows.h>
#include <iostream>
#include <olectl.h>
#include <iomanip>
#include <vector>
#include <memory>
#include <thread>

#include "Core/Texture.h"
#include "Core/Integrator.h"
#include "Utils/FrameBufferDouble.h"

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
	std::string mName;
	int mWindowWidth;
	int mWindowHeight;

	HINSTANCE mInstance;
	HWND mWindow;
	HDC mMemdc;
	HBITMAP mBitmap;

	bool mKeyPressing[256];
	bool mF1Pressed = false;
	bool mCursorDisabled = true;
	int mLastCursorX;
	int mLastCursorY;
	bool mFirstCursorMove = true;

	int mToneMappingMethod = 2;

	FrameBufferDouble<RGB24> mColorBuffer;
	IntegratorPtr mIntegrator;
	ScenePtr mScene;

	float mResultScale = 1.0f;
};
