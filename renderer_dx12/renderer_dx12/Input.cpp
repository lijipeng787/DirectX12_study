#include "stdafx.h"

#include "Input.h"

bool Input::Initialize(HINSTANCE hinstance, HWND hwnd, int screen_width,
                       int screen_height) {

  // Store the screen size which will be used for positioning the mouse cursor.
  screen_width_ = screen_width;
  screen_height_ = screen_height;

  // Initialize the location of the mouse on the screen.
  mouse_x_ = 0;
  mouse_y_ = 0;

  // Initialize the main direct input interface.
  if (FAILED(DirectInput8Create(hinstance, DIRECTINPUT_VERSION,
                                IID_IDirectInput8, (void **)&directInput_,
                                NULL))) {
    return false;
  }

  // Initialize the direct input interface for the keyboard.
  if (FAILED(directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL))) {
    return false;
  }

  // Set the data format.  In this case since it is a keyboard we can use the
  // predefined data format.
  if (FAILED(keyboard_->SetDataFormat(&c_dfDIKeyboard))) {
    return false;
  }

  // Set the cooperative level of the keyboard to share with other programs.
  if (FAILED(keyboard_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND |
                                                      DISCL_NONEXCLUSIVE))) {
    return false;
  }

  // Now acquire the keyboard.
  if (FAILED(keyboard_->Acquire())) {
    return false;
  }

  // Initialize the direct input interface for the mouse.
  if (FAILED(directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL))) {
    return false;
  }

  // Set the data format for the mouse using the pre-defined mouse data format.
  if (FAILED(mouse_->SetDataFormat(&c_dfDIMouse))) {
    return false;
  }

  // Set the cooperative level of the mouse to share with other programs.
  if (FAILED(mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND |
                                                   DISCL_NONEXCLUSIVE))) {
    return false;
  }

  // Acquire the mouse.
  if (FAILED(mouse_->Acquire())) {
    return false;
  }

  return true;
}

void Input::Shutdown() {
  // Release the mouse.
  if (mouse_) {
    mouse_->Unacquire();
    mouse_->Release();
    mouse_ = nullptr;
  }

  // Release the keyboard.
  if (keyboard_) {
    keyboard_->Unacquire();
    keyboard_->Release();
    keyboard_ = nullptr;
  }

  // Release the main interface to direct input.
  if (directInput_) {
    directInput_->Release();
    directInput_ = nullptr;
  }
}

bool Input::Update() {

  // Read the current state of the keyboard.
  if (FAILED(ReadKeyboard())) {
    return false;
  }

  // Read the current state of the mouse.
  if (FAILED(ReadMouse())) {
    return false;
  }

  // Process the changes in the mouse and keyboard.
  ProcessInput();

  return true;
}

bool Input::ReadKeyboard() {

  // Read the keyboard device.
  auto result = keyboard_->GetDeviceState(sizeof(keyboard_state_),
                                          (LPVOID)&keyboard_state_);
  if (FAILED(result)) {
    // If the keyboard lost focus or was not acquired then try to get control
    // back.
    if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
      keyboard_->Acquire();
    } else {
      return false;
    }
  }

  return true;
}

bool Input::ReadMouse() {

  // Read the mouse device.
  auto result =
      mouse_->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&mouse_state_);
  if (FAILED(result)) {
    // If the mouse lost focus or was not acquired then try to get control back.
    if ((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED)) {
      mouse_->Acquire();
    } else {
      return false;
    }
  }

  return true;
}

void Input::ProcessInput() {

  // Update the location of the mouse cursor based on the change of the mouse
  // location during the frame.
  mouse_x_ += mouse_state_.lX;
  mouse_y_ += mouse_state_.lY;

  // Ensure the mouse location doesn't exceed the screen width or height.
  if (mouse_x_ < 0) {
    mouse_x_ = 0;
  }
  if (mouse_y_ < 0) {
    mouse_y_ = 0;
  }

  if (mouse_x_ > screen_width_) {
    mouse_x_ = screen_width_;
  }
  if (mouse_y_ > screen_height_) {
    mouse_y_ = screen_height_;
  }
}

bool Input::IsEscapePressed() {
  if (keyboard_state_[DIK_ESCAPE] & 0x80)
    return true;
  return false;
}

bool Input::IsLeftPressed() {
  if (keyboard_state_[DIK_LEFT] & 0x80)
    return true;
  return false;
}

bool Input::IsRightPressed() {
  if (keyboard_state_[DIK_RIGHT] & 0x80)
    return true;
  return false;
}

bool Input::IsUpPressed() {
  if (keyboard_state_[DIK_UP] & 0x80)
    return true;
  return false;
}

bool Input::IsDownPressed() {
  if (keyboard_state_[DIK_DOWN] & 0x80)
    return true;
  return false;
}

bool Input::IsAPressed() {
  if (keyboard_state_[DIK_A] & 0x80)
    return true;
  return false;
}

bool Input::IsZPressed() {
  if (keyboard_state_[DIK_Z] & 0x80)
    return true;
  return false;
}

bool Input::IsPageUpPressed() {
  if (keyboard_state_[DIK_PGUP] & 0x80)
    return true;
  return false;
}

bool Input::IsPageDownPressed() {
  if (keyboard_state_[DIK_PGDN] & 0x80)
    return true;
  return false;
}