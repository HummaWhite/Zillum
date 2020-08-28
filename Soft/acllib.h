#ifndef __ACLLIB_H__
#define __ACLLIB_H__

#ifdef _UNICODE
#undef _UNICODE
#endif
#ifdef UNICODE
#undef UNICODE
#endif

#include <Windows.h>

#define EMPTY				0xffffffff
#define DEFAULT				-1

typedef enum
{
	NO_BUTTON = 0,
	LEFT_BUTTON,
	MIDDLE_BUTTON,
	RIGHT_BUTTON
} ACL_Mouse_Button;

typedef enum
{
	BUTTON_DOWN,
	BUTTON_DOUBLECLICK,
	BUTTON_UP,
	ROLL_UP,
	ROLL_DOWN,
	MOUSEMOVE
} ACL_Mouse_Event;

typedef enum
{
	KEY_DOWN,
	KEY_UP
} ACL_Keyboard_Event;

typedef void(*KeyboardEventCallback) (int key, int event);
typedef void(*CharEventCallback) (char c);
typedef void(*MouseEventCallback) (int x, int y, int button, int event);
typedef void(*TimerEventCallback) (int timerID);

#ifdef __cplusplus
extern "C" {
#endif

	int Setup(void);

	void initWindow(const char *title, int left, int top, int width, int height);
	void msgBox(const char *title, const char *text, int flag);

	void registerKeyboardEvent(KeyboardEventCallback callback);
	void registerCharEvent(CharEventCallback callback);
	void registerMouseEvent(MouseEventCallback callback);
	void registerTimerEvent(TimerEventCallback callback);

	void startTimer(int timerID, int timeinterval);
	void cancelTimer(int timerID);

	int getWidth();
	int getHeight();

	void flushScreen(BYTE *buf, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
