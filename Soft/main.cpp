#include <windows.h>

#include "Application.h"

Application app;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return app.process(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	const char *name = "Test";

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

	int width, height;
	std::cout << "Input Window Size\n";
	std::cin >> width >> height;
	app.init(std::string(name), hInstance, width, height);

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
			app.render();
		}
	}

	return msg.wParam;
}
