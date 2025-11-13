#pragma once

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

  void GetMouseLocation(int &mouse_x, int &mouse_y) const {
    mouse_x = mouse_x_;
    mouse_y = mouse_y_;
  }

  auto IsEscapePressed() const -> bool;

  auto IsLeftPressed() const -> bool;

  auto IsRightPressed() const -> bool;

  auto IsUpPressed() const -> bool;

  auto IsDownPressed() const -> bool;

  auto IsWPressed() const -> bool;

  auto IsSPressed() const -> bool;

  auto IsAPressed() const -> bool;

  auto IsDPressed() const -> bool;

  auto IsZPressed() const -> bool;

  auto IsPageUpPressed() const -> bool;

  auto IsPageDownPressed() const -> bool;

  auto IsRPressed() const -> bool;

  auto IsFPressed() const -> bool;

  auto IsQPressed() const -> bool;

  auto IsEPressed() const -> bool;

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
