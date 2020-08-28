#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define CINTERFACE

#ifdef _UNICODE
#undef _UNICODE
#endif
#ifdef UNICODE
#undef UNICODE
#endif

#include "acllib.h"

#include <windows.h>
#include <olectl.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma comment(lib,"winmm.lib")
#pragma comment(lib,"msimg32.lib")
#endif

#ifdef _DEBUG
#define ACL_ASSERT(_Expression,errStr) (void)( (!!(_Expression)) || (acl_error(errStr),0) )
#else
#define ACL_ASSERT(flag,errStr) ((void)0)
#endif

#define ACL_ASSERT_HWND ACL_ASSERT(g_hWnd!=0, \
		"You should call function \"initWindow(...)\" befor use function \"" __FUNCTION__ "\"" )
#define ACL_ASSERT_BEGIN_PAINT ACL_ASSERT(g_hmemdc!=0, \
		"You should call function \"beginPaint()\" befor use function \"" __FUNCTION__ "\"" )

// f
int Setup(void);

const char g_wndClassName[] = "ACL_WND_CLASS";
const char g_libName[] = "ACLLIB";

HINSTANCE g_hInstance;

HWND g_hWnd = NULL;
HDC g_hmemdc = NULL;
HBITMAP g_hbitmap = NULL;

int g_wndHeight;
int g_wndWidth;

KeyboardEventCallback g_keyboard = NULL;
MouseEventCallback g_mouse = NULL;
TimerEventCallback g_timer = NULL;
CharEventCallback g_char = NULL;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//
void acl_error(char *errStr)
{
	MessageBoxA(g_hWnd, errStr, g_libName, MB_ICONERROR);
	exit(0);
}

//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASSA     wndclass;

	g_hInstance = hInstance;
	g_hWnd = NULL;
	g_keyboard = NULL;
	g_mouse = NULL;
	g_timer = NULL;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = g_wndClassName;

	if (!RegisterClassA(&wndclass))
	{
		MessageBoxA(NULL, "This program requires Windows NT!", g_libName, MB_ICONERROR);
		return 0;
	}

	Setup();

	ACL_ASSERT(g_hWnd,"You must call \"initWindow(...)\" in Main()");

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

//
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{ 
	switch (message) 
	{
		case WM_CREATE:
		{
			HDC hdc;
			hdc = GetDC(hwnd);
			g_hbitmap = CreateCompatibleBitmap
			(
				hdc,
				GetSystemMetrics(SM_CXSCREEN),
				GetSystemMetrics(SM_CYSCREEN)
			);

			g_hmemdc = CreateCompatibleDC(hdc);
			SelectObject(g_hmemdc, g_hbitmap);
			
			BitBlt
			(
				g_hmemdc, 
				0, 0, 
				GetSystemMetrics(SM_CXSCREEN),
				GetSystemMetrics(SM_CYSCREEN), 
				g_hmemdc, 
				0, 0, 
				WHITENESS
			);

			DeleteDC(g_hmemdc);
			ReleaseDC(hwnd, hdc);

			break;
		}

		case WM_ERASEBKGND:
			break;

		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT ps;
			RECT rect;

			hdc = BeginPaint(hwnd, &ps);
			g_hmemdc = CreateCompatibleDC(hdc);
			SelectObject(g_hmemdc, g_hbitmap);
			GetClientRect(hwnd, &rect);

			BitBlt
			(
			 	hdc,
				0, 0,
				rect.right - rect.left, rect.bottom - rect.top,
				g_hmemdc,
				0, 0,
				SRCCOPY
			);

			DeleteDC(g_hmemdc);
			g_hmemdc = 0;
			EndPaint(hwnd,&ps);

			break;
		}

		case WM_CHAR:
			if (g_char != NULL)	g_char((char)wParam);
			break;

		case WM_KEYDOWN:
			if (g_keyboard != NULL) g_keyboard((int)wParam, KEY_DOWN);
			break;

		case WM_KEYUP:
			if(g_keyboard != NULL) g_keyboard((int)wParam, KEY_UP);
			break;

		case WM_LBUTTONDOWN:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), LEFT_BUTTON, BUTTON_DOWN);
			}
			break;

		case WM_LBUTTONUP:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), LEFT_BUTTON, BUTTON_UP);
			}
			break;

		case WM_LBUTTONDBLCLK:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), LEFT_BUTTON, BUTTON_DOUBLECLICK);
			}
			break;

		case WM_MBUTTONDOWN:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MIDDLE_BUTTON, BUTTON_DOWN);
			}
			break;

		case WM_MBUTTONUP:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MIDDLE_BUTTON, BUTTON_UP);
			}
			break;

		case WM_MBUTTONDBLCLK:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MIDDLE_BUTTON, BUTTON_DOUBLECLICK);
			}
			break;

		case WM_RBUTTONDOWN:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), RIGHT_BUTTON, BUTTON_DOWN);
			}
			break;

		case WM_RBUTTONUP:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), RIGHT_BUTTON, BUTTON_UP);
			}
			break;

		case WM_RBUTTONDBLCLK:
			if (g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), RIGHT_BUTTON, BUTTON_DOUBLECLICK);
			}
			break;

		case WM_MOUSEMOVE:
			if(g_mouse != NULL)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MOUSEMOVE, MOUSEMOVE);
			}
			break;

		case WM_MOUSEWHEEL:
			if(g_mouse == NULL) break;

			if(HIWORD(wParam) == 120)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MIDDLE_BUTTON, ROLL_UP);
			}
			else if(HIWORD(wParam)==65416)
			{
				g_mouse((int)LOWORD(lParam), (int)HIWORD(lParam), MIDDLE_BUTTON, ROLL_DOWN);
			}
			break;

		case WM_TIMER:
			if (g_timer != NULL) g_timer(wParam);
			break;

		case WM_DESTROY:
			DeleteObject(g_hbitmap);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

//
void initWindow(const char *wndName, int x, int y, int width, int height)
{
	RECT rect;

	ACL_ASSERT(!g_hWnd,"Don't call initWindow twice");

	g_wndHeight = height;
	g_wndWidth = width;

	if(x == DEFAULT || y == DEFAULT)
	{
		x = y = CW_USEDEFAULT;
	}

	g_hWnd = CreateWindowA
	(
		g_wndClassName, wndName,
		WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX,
		x, y, 
		width, height,
		NULL, NULL, 0, NULL
	);

	if(!g_hWnd)
	{
		MessageBoxA(NULL, "Fail to create window", g_libName, MB_ICONERROR);
		exit(0);
	}

	GetClientRect(g_hWnd, &rect);
	width += width - (rect.right - rect.left);
	height += height - (rect.bottom - rect.top);
	SetWindowPos
	(
	 	g_hWnd,
		HWND_TOP,
		0, 0,
		width, height,
		SWP_NOMOVE
	);

	ShowWindow(g_hWnd, 1);
	UpdateWindow(g_hWnd);
}

void msgBox(const char title[], const char text[], int flag)
{
	ACL_ASSERT_HWND;
	MessageBoxA(g_hWnd, text, title, flag);
}

int getWidth(void)
{
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	return rect.right;
}

int getHeight(void)
{
	RECT rect;
	GetClientRect(g_hWnd, &rect);
	return rect.bottom;
}

void flushScreen(BYTE *buf, int width, int height)
{
	HDC screen_hdc;
	HWND screen_hwnd;
	HDC hCompatibleDC; //兼容HDC
	HBITMAP hCompatibleBitmap; //兼容BITMAP
	HBITMAP hOldBitmap; //旧的BITMAP				  
	BITMAPINFO binfo; //BITMAPINFO结构体
 
  	ZeroMemory(&binfo, sizeof(BITMAPINFO));
  	binfo.bmiHeader.biBitCount = 24;      //每个像素多少位，也可直接写24(RGB)或者32(RGBA)
  	binfo.bmiHeader.biCompression = BI_RGB;
  	binfo.bmiHeader.biHeight = -height;
  	binfo.bmiHeader.biPlanes = 1;
  	binfo.bmiHeader.biSizeImage = 0;
  	binfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  	binfo.bmiHeader.biWidth = width;
 
 
 //获取屏幕HDC
 	screen_hwnd = g_hWnd;
	screen_hdc = GetDC(screen_hwnd);
	
 	//获取兼容HDC和兼容Bitmap,兼容Bitmap选入兼容HDC(每个HDC内存每时刻仅能选入一个GDI资源,GDI资源要选入HDC才能进行绘制)
 	hCompatibleDC = CreateCompatibleDC(screen_hdc);
 	hCompatibleBitmap = CreateCompatibleBitmap(screen_hdc, width, height);
 	hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hCompatibleBitmap);
 	//将颜色数据打印到屏幕上，这下面两个函数每帧都得调用
  	SetDIBits(screen_hdc, hCompatibleBitmap, 0, height, buf, (BITMAPINFO*)&binfo, DIB_RGB_COLORS);
   	BitBlt(screen_hdc, -1, -1, width, height, hCompatibleDC, 0, 0, SRCCOPY);

	//ReleaseDC(screen_hwnd, screen_hdc);
	DeleteDC(hCompatibleDC);
	DeleteObject(hOldBitmap);
	DeleteObject(hCompatibleBitmap);

	UpdateWindow(g_hWnd);
}

void registerKeyboardEvent(KeyboardEventCallback callback)
{
	g_keyboard = callback;
}

void registerCharEvent(CharEventCallback callback)
{
	g_char = callback;
}

void registerMouseEvent(MouseEventCallback callback)
{
	g_mouse = callback;
}

void registerTimerEvent(TimerEventCallback callback)
{
	g_timer = callback;
}

void startTimer(int id,int timeinterval)
{
	SetTimer(g_hWnd, id, timeinterval, NULL);
}

void cancelTimer(int id)
{
	KillTimer(g_hWnd, id);
}

