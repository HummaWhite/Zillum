#include <windows.h>

#include "Zillum.h"

Zillum app;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return app.process(hWnd, message, wParam, lParam);
}

int main(int argc, char* argv[]) {
	const char* name = "Zillum";

	MSG				msg;
	WNDCLASSA		wndClass;
	HINSTANCE		instance = GetModuleHandle(nullptr);

	wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wndClass.lpfnWndProc = WndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = instance;
	wndClass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = name;

	if (!RegisterClassA(&wndClass)) {
		MessageBoxA(nullptr, "This program requires Windows NT!", name, MB_ICONERROR);
		return 0;
	}

	std::string cmdLine;
	for (int i = 1; i < argc; i++) {
		cmdLine += argv[i] + ' ';
	}

	std::cout << cmdLine << std::endl;
	app.init(std::string(name), instance, cmdLine.c_str());

	while (true) {
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				break;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if (!app.render()) {
				break;
			}
		}
	}
	return msg.wParam;
}