#include <windows.h>

#include "../include/Application.h"

Application app;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return app.process(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	const char *name = "Zillum";

	MSG				msg;
	WNDCLASSA		wndClass;

	wndClass.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wndClass.lpfnWndProc	= WndProc;
	wndClass.cbClsExtra		= 0;
	wndClass.cbWndExtra		= 0;
	wndClass.hInstance		= hInstance;
	wndClass.hIcon			= LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName	= nullptr;
	wndClass.lpszClassName	= name;

	if (!RegisterClassA(&wndClass))
	{
		MessageBoxA(nullptr, "This program requires Windows NT!", name, MB_ICONERROR);
		return 0;
	}

	app.init(std::string(name), hInstance, szCmdLine);

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) break;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			if (!app.render())
				break;
		}
	}
	// ThinLensCamera camera(90.0f);
	// camera.setPos(Vec3f(0.0f));
	// camera.lookAt({ 0.0f, 1.0f, 0.0f });
	// camera.initFilm(100, 100);
	// MetalWorkflow bsdf(Vec3f(1.0f), 1.0f, 0.1f);

	// Vec3f AL = glm::normalize(Vec3f(-1.0f, 1.0f, 0.0f));
	// Vec3f AB = glm::normalize(Vec3f(1.0f, 1.0f, 0.0f));
	// Vec3f BA = glm::normalize(Vec3f(-1.0f, 1.0f, 0.0f));
	// Vec3f BC = glm::normalize(Vec3f(-2.0f, 1.0f, 0.0f));
	// Vec3f CB = glm::normalize(Vec3f(1.0f, 2.0f, 0.0f));
	// Vec3f CE = glm::normalize(Vec3f(-1.0f, 2.0f, 0.0f));
	// Vec3f N = glm::normalize(Vec3f(0.0f, 1.0f, 0.0f));

	// std::cout << Math::vec3ToString(bsdf.bsdf(N, AL, AB, TransportMode::Radiance)) << "\n";
	// std::cout << Math::vec3ToString(bsdf.bsdf(N, AB, AL, TransportMode::Importance)) << "\n";
	// std::cout << bsdf.pdf(N, AL, AB, TransportMode::Radiance) << "\n";
	// std::cout << bsdf.pdf(N, AB, AL, TransportMode::Importance) << "\n";

	// std::cout << Math::vec3ToString(bsdf.bsdf(N, CB, CE, TransportMode::Radiance)) << "\n";
	// std::cout << Math::vec3ToString(bsdf.bsdf(N, CE, CB, TransportMode::Importance)) << "\n";
	// std::cout << bsdf.pdf(N, CB, CE, TransportMode::Radiance) << "\n";
	// std::cout << bsdf.pdf(N, CE, CB, TransportMode::Importance) << "\n";

	// Ray ray(Vec3f(0.0f), glm::normalize(Vec3f(1.0f, 2.0f, 0.0f)));
	// std::cout << camera.pdfIe(ray).pdfDir << "\n";

	return msg.wParam;
}
