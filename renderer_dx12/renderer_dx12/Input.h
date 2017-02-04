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

	bool IsEscapePressed() { if (keyboard_state_[DIK_ESCAPE] & 0x80) return true; return false; }

	bool IsLeftPressed() { if (keyboard_state_[DIK_LEFT] & 0x80) return true; return false; }

	bool IsRightPressed() { if (keyboard_state_[DIK_RIGHT] & 0x80) return true; return false; }

	bool IsUpPressed() { if (keyboard_state_[DIK_UP] & 0x80) return true; return false; }

	bool IsDownPressed() { if (keyboard_state_[DIK_DOWN] & 0x80) return true; return false; }

	bool IsAPressed() { if (keyboard_state_[DIK_A] & 0x80) return true; return false; }

	bool IsZPressed() { if (keyboard_state_[DIK_Z] & 0x80) return true; return false; }

	bool IsPageUpPressed() { if (keyboard_state_[DIK_PGUP] & 0x80) return true; return false; }

	bool IsPageDownPressed() { if (keyboard_state_[DIK_PGDN] & 0x80) return true; return false; }
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