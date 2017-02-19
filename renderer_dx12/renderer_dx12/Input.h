#ifndef _INPUTCLASS_H_
#define _INPUTCLASS_H_

#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"dxguid.lib")

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>

class Input {
public:
	Input() {}

	Input(const Input& rhs) = delete;

	Input& operator=(const Input& rhs) = delete;

	~Input() {}
public:
	bool Initialize(HINSTANCE hinstance, HWND hwnd, int screen_width, int screen_height);

	void Shutdown();

	bool Update();

	void GetMouseLocation(int& mouse_x, int& mouse_y) { mouse_x = mouse_x_; mouse_y = mouse_y_; }

	bool IsEscapePressed();

	bool IsLeftPressed();

	bool IsRightPressed(); 

	bool IsUpPressed();

	bool IsDownPressed();

	bool IsAPressed();

	bool IsZPressed();

	bool IsPageUpPressed();

	bool IsPageDownPressed();
private:
	void ProcessInput();

	bool ReadKeyboard();

	bool ReadMouse();
private:
	IDirectInput8* directInput_ = nullptr;

	IDirectInputDevice8* keyboard_ = nullptr;

	IDirectInputDevice8* mouse_ = nullptr;

	int screen_width_ = {};

	int screen_height_ = {};

	int mouse_location_x_ = {};

	int mouse_location_y_ = {};

	unsigned char keyboard_state_[256] = { 0 };

	DIMOUSESTATE mouse_state_ = {};

	int mouse_x_ = 0, mouse_y_ = 0;
};

#endif