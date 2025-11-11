#ifndef _INPUTCLASS_H_
#define _INPUTCLASS_H_

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>
#include <wrl/client.h>

class Input {
public:
  Input() = default;

  Input(const Input &rhs) = delete;

  auto operator=(const Input &rhs) -> Input & = delete;

  ~Input() {}

public:
  auto Initialize(HINSTANCE hinstance, HWND hwnd, int screen_width,
                  int screen_height) -> bool;

  void Shutdown();

  auto Update() -> bool;

  void GetMouseLocation(int &mouse_x, int &mouse_y) {
    mouse_x = mouse_x_;
    mouse_y = mouse_y_;
  }

  auto IsEscapePressed() -> bool;

  auto IsLeftPressed() -> bool;

  auto IsRightPressed() -> bool;

  auto IsUpPressed() -> bool;

  auto IsDownPressed() -> bool;

  auto IsWPressed() -> bool;

  auto IsSPressed() -> bool;

  auto IsAPressed() -> bool;

  auto IsDPressed() -> bool;

  auto IsZPressed() -> bool;

  auto IsPageUpPressed() -> bool;

  auto IsPageDownPressed() -> bool;

  auto IsRPressed() -> bool;

  auto IsFPressed() -> bool;

  auto IsQPressed() -> bool;

  auto IsEPressed() -> bool;

private:
  void ProcessInput();

  auto ReadKeyboard() -> bool;

  auto ReadMouse() -> bool;

private:
  Microsoft::WRL::ComPtr<IDirectInput8> directInput_;

  Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;

  Microsoft::WRL::ComPtr<IDirectInputDevice8> mouse_;

  int screen_width_ = {};

  int screen_height_ = {};

  int mouse_location_x_ = {};

  int mouse_location_y_ = {};

  unsigned char keyboard_state_[256] = {0};

  DIMOUSESTATE mouse_state_ = {};

  int mouse_x_ = 0, mouse_y_ = 0;
};

#endif