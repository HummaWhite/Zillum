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
#include "Utils/ImageSave.h"
#include "SceneLoader.h"

class Application
{
public:
	void init(const std::string& name, HINSTANCE instance, const char *cmdParam);
	LRESULT process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	bool render();

private:
	void initScene();
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

	bool mToneMapping = true;
	bool mCorrectGamma = true;

	bool mAutoSaveImage = true;

	FrameBufferDouble<RGB24> mColorBuffer;
	IntegratorPtr mIntegrator;
	ScenePtr mScene;
};
