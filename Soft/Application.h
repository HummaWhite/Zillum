#ifndef APPLICATION_H
#define APPLICATION_H

#pragma GCC optimize(3, "Ofast", "inline")

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

#include "Texture.h"
#include "FrameBufferDouble.h"
#include "Integrator/Whitted.h"
#include "Integrator/Bidirectional.h"
#include "Integrator/AmbientOcclusion.h"

class Application
{
public:
	~Application() {}

	void init(const std::string& name, HINSTANCE instance, int width, int height)
	{
		this->instance = instance;
		this->windowWidth = width;
		this->windowHeight = height;

		lastCursorX = width / 2;
		lastCursorY = height / 2;
		
		window = CreateWindowA(
			name.c_str(), name.c_str(),
			WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_SIZEBOX,
			CW_USEDEFAULT, CW_USEDEFAULT,
			width, height,
			nullptr,
			nullptr,
			0,
			nullptr
		);

		SetWindowPos(window, HWND_TOP, 0, 0, width, height, SWP_NOMOVE);
		ShowWindow(window, 1);
		UpdateWindow(window);

		colorBuffer.init(width, height);
		initScene();
	}

	LRESULT process(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
		{
			auto metricX = GetSystemMetrics(SM_CXSCREEN);
			auto metricY = GetSystemMetrics(SM_CYSCREEN);

			HDC hdc = GetDC(hWnd);
			bitmap = CreateCompatibleBitmap(hdc, metricX, metricY);

			memdc = CreateCompatibleDC(hdc);
			SelectObject(memdc, bitmap);
			
			BitBlt(memdc, 0, 0, metricX, metricY, memdc, 0, 0, WHITENESS);

			DeleteDC(memdc);
			ReleaseDC(hWnd, hdc);

			break;
		}

		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			RECT rect;

			hdc = BeginPaint(hWnd, &ps);
			memdc = CreateCompatibleDC(hdc);
			SelectObject(memdc, bitmap);
			GetClientRect(hWnd, &rect);

			BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, memdc, 0, 0, SRCCOPY);

			DeleteDC(memdc);
			memdc = 0;
			EndPaint(hWnd, &ps);

			break;
		}

		case WM_KEYDOWN:
		{
			keyPressing[(int)wParam] = true;
			if ((int)wParam == VK_F1) cursorDisabled ^= 1;
			if ((int)wParam == 'T') toneMappingMethod = (toneMappingMethod + 1) % 4;
			else if ((int)wParam == 'O') saveImage();
			break;
		}

		case WM_KEYUP:
			keyPressing[(int)wParam] = false;
			break;

		case WM_MOUSEMOVE:
		{
			if (cursorDisabled) break;
			integrator->modified = true;

			if (firstCursorMove)
			{
				lastCursorX = (int)LOWORD(lParam);
				lastCursorY = (int)HIWORD(lParam);
				firstCursorMove = false;
			}

			float offsetX = ((int)LOWORD(lParam) - lastCursorX) * CAMERA_ROTATE_SENSITIVITY;
    		float offsetY = ((int)HIWORD(lParam) - lastCursorY) * CAMERA_ROTATE_SENSITIVITY;

    		glm::vec3 offset(-offsetX, -offsetY, 0.0f);
    		camera->rotate(offset);

   			lastCursorX = (int)LOWORD(lParam);
    		lastCursorY = (int)HIWORD(lParam);
			break;
		}

		case WM_DESTROY:
			DeleteObject(bitmap);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	void render()
	{
		processKey();
		integrator->render();
		writeBuffer();
		flushScreen();
		colorBuffer.swap();
	}

private:
	void initScene()
	{
		srand(time(nullptr));

		std::vector<std::shared_ptr<Shape>> shapeList;
		std::vector<std::shared_ptr<Light>> lightList;

		shapeList.push_back
		(
			std::make_shared<Quad>
			(
			  	glm::vec3(-200.0f, -10.0f, -2.0f),
			  	glm::vec3(200.0f, -10.0f, -2.0f),
			  	glm::vec3(-200.0f, 100.0f, -2.0f),
			  	std::make_shared<MaterialPBR>(glm::vec3(1.0f), 0.0f, 1.0f)
			)
		);

		RandomGenerator rg;
		rg.srand(time(0));

		auto modelInfo = ObjReader::readFile("res/model/dragon2.obj");
		glm::mat4 model(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f));
		std::shared_ptr<Transform> trTeapot = std::make_shared<Transform>(model);

		int faceCount = modelInfo.vertices.size() / 3;
		for (int i = 0; i < faceCount; i++)
		{
			glm::vec3 v[] = { modelInfo.vertices[i * 3 + 0], modelInfo.vertices[i * 3 + 1], modelInfo.vertices[i * 3 + 2] };
			glm::vec3 n[] = { modelInfo.normals[i * 3 + 0], modelInfo.normals[i * 3 + 1], modelInfo.normals[i * 3 + 2] };
			glm::vec2 t[3];

			std::shared_ptr<MeshTriangle> tr = std::make_shared<MeshTriangle>(
				v, t, n,
				std::make_shared<MaterialPBR>(glm::vec3(1.0f, 0.71f, 0.29f), 1.0f, 0.15f) // 1.0 0.86 0.57
				//std::make_shared<Dieletric>(1.5f)
			);

			tr->setTransform(trTeapot);
			shapeList.push_back(tr);
		}

		lightList.push_back
		(
			std::make_shared<QuadLight>
			(
			  	glm::vec3(-2.0f, 2.5f, 2.99f),
			  	glm::vec3(4.0f, 2.5f, 2.99f),
			  	glm::vec3(-2.0f, 0.5f, 2.99f),
			  	glm::vec3(100.0f)
			)
		);

		camera->setPos({ -2.0f, -3.0f, 1.25f });
		camera->setFOV(75.0f);
		camera->lookAt(glm::vec3(0.0f, 0.0f, -0.5f));

		auto env = std::make_shared<EnvSphereMapHDR>("res/texture/024.hdr");
		//env = std::make_shared<EnvSingleColor>(glm::vec3(0.0f));
		auto scene = std::make_shared<Scene>(shapeList, lightList, env, camera);
		scene->buildBVH();
		auto integ = std::make_shared<AOIntegrator>(windowWidth, windowHeight, 64);
		integ->occlusionRadius = glm::vec3(1.0f, 2.0f, 3.0f);
		//integ->lowDiscrepSeries = true;
		integ->setScene(scene);
		//integ->roulette = true;
		//integ->rouletteProb = 0.8f;
		integrator = integ;
	}

	void writeBuffer()
	{
		auto resultBuffer = integrator->result();
		using namespace ToneMapping;

		glm::vec3(*toneMapping[4])(const glm::vec3&) = { reinhard, CE, filmic, ACES };

		for (int i = 0; i < windowWidth; i++)
		{
			for (int j = 0; j < windowHeight; j++)
			{
				auto result = resultBuffer(i, j);
				result = glm::clamp(result, glm::vec3(0.0f), glm::vec3(1e8f));
				result = toneMapping[toneMappingMethod](result);
				result = glm::pow(result, glm::vec3(1.0f / 2.2f));
				colorBuffer(i, j) = RGB24::swapRB(RGB24(result));
			}
		}
	}

	void flushScreen()
	{
		BITMAPINFO bInfo;
		ZeroMemory(&bInfo, sizeof(BITMAPINFO));

		auto &header = bInfo.bmiHeader;
		header.biBitCount		= 24;
		header.biCompression	= BI_RGB;
		header.biWidth			= windowWidth;
		header.biHeight			= -windowHeight;
		header.biPlanes			= 1;
		header.biSizeImage		= 0;
		header.biSize			= sizeof(BITMAPINFOHEADER);

		HDC dc = GetDC(window);
		HDC compatibleDC = CreateCompatibleDC(dc);
		HBITMAP compatibleBitmap = CreateCompatibleBitmap(dc, windowWidth, windowHeight);
		HBITMAP oldBitmap = (HBITMAP)SelectObject(compatibleDC, compatibleBitmap);

		SetDIBits
		(
			dc,
			compatibleBitmap,
			0,
			windowHeight,
			(BYTE*)colorBuffer.getCurrentBuffer().bufPtr(),
			&bInfo,
			DIB_RGB_COLORS
		);
		BitBlt(dc, -1, -1, windowWidth, windowHeight, compatibleDC, 0, 0, SRCCOPY);

		DeleteDC(compatibleDC);
		DeleteObject(oldBitmap);
		DeleteObject(compatibleBitmap);
		UpdateWindow(window);
	}

	void processKey()
	{
		unsigned char keyList[] = { 'W', 'S', 'A', 'D', 'Q', 'E', 'R', VK_SHIFT, VK_SPACE };

		for (int i = 0; i < 9; i++)
		{
			if (keyPressing[keyList[i]])
			{
				camera->move(keyList[i]);
				integrator->modified = true;
			}
		}
	}

	void saveImage()
	{
		int w = colorBuffer.width(), h = colorBuffer.height();
		int size = w * h;
		RGB24 *data = new RGB24[size];
		for (int i = 0; i < size; i++) data[i] = RGB24::swapRB(colorBuffer[i]);
		std::string file = "screenshot/saves/save" + std::to_string((int)time(0)) + ".png";
		stbi_write_png(file.c_str(), w, h, 3, data, w * 3);
		delete[] data;
	}

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
	std::shared_ptr<Integrator> integrator;
	std::shared_ptr<Camera> camera = std::make_shared<Camera>(glm::vec3(-0.0f, -3.0f, 0.0f));
};

#endif
