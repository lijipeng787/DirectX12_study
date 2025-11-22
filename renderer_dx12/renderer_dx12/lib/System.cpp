#include "stdafx.h"

#include "System.h"

#include <iostream>

#include "Input.h"
#include "Timer.h"
#include "Graphics.h"

bool System::Initialize() {
  int screenWidth, screenHeight;
  bool result;

  // Initialize the width and height of the screen to zero before sending the
  // variables into the function.
  screenWidth = 800;
  screenHeight = 600;

  // Initialize the windows api.
  InitializeWindows(screenWidth, screenHeight);

  // Create the input object.  This object will be used to handle reading the
  // keyboard input from the user.
  input_ = std::make_unique<Input>();
  if (!input_) {
    return false;
  }

  // Initialize the input object.
  if (!input_->Initialize(hinstance_, hwnd_, screenWidth, screenHeight)) {
    return false;
  }

  // Create and initialize the high resolution timer.
  timer_ = std::make_unique<TimerClass>();
  if (!timer_) {
    return false;
  }

  if (!timer_->Initialize()) {
    return false;
  }

  // Create the graphics object.  This object will handle rendering all the
  // graphics for this application.
  graphics_ = std::make_unique<Graphics>();
  if (!graphics_) {
    return false;
  }

  // Initialize the graphics object.
  result = graphics_->Initialize(screenWidth, screenHeight, hwnd_);
  if (!result) {
    return false;
  }

  return true;
}

void System::Shutdown() {
  // Release the graphics object.
  if (graphics_) {
    graphics_->Shutdown();
    graphics_.reset();
  }

  // Release the input object.
  if (input_) {
    input_.reset();
  }

  if (timer_) {
    timer_.reset();
  }

  // Shutdown the window.
  ShutdownWindows();
}

void System::Run() {
  MSG msg;
  bool done, result;

  // Initialize the message structure.
  ZeroMemory(&msg, sizeof(MSG));

  // Loop until there is a quit message from the window or the user.
  done = false;
  while (!done) {
    // Handle the windows messages.
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // If windows signals to end the application then exit out.
    if (msg.message == WM_QUIT) {
      done = true;
    } else {
      // Otherwise do the frame processing.
      result = Frame();
      if (!result) {
        done = true;
      }
    }
  }
}

bool System::Frame() {
  // Skip rendering if we're in the middle of a resize operation
  if (is_resizing_) {
    return true;
  }

  float delta_seconds = 0.0f;
  if (timer_) {
    timer_->Update();
    delta_seconds = timer_->GetTime() * 0.001f;
  }

  if (input_) {
    if (!input_->Update()) {
      return false;
    }

    if (input_->IsEscapePressed()) {
      return false;
    }
  }

  if (!graphics_->Frame(delta_seconds, input_.get())) {
    return false;
  }

  return true;
}

void System::OnResize(int new_width, int new_height) {
  if (!graphics_) {
    return;
  }

  Logger::Info(L"[System::OnResize] Resizing to %dx%d", new_width, new_height);

  if (!graphics_->OnResize(new_width, new_height)) {
    Logger::Error(L"[System::OnResize] Failed to resize graphics!");
  }
}

LRESULT CALLBACK System::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam,
                                        LPARAM lparam) {
  switch (umsg) {
  // Check if a key has been pressed on the keyboard.
  case WM_KEYDOWN: {
    break;
  }

  // Check if a key has been released on the keyboard.
  case WM_KEYUP: {
    break;
  }

  // Handle window resize
  case WM_SIZE: {
    int new_width = LOWORD(lparam);
    int new_height = HIWORD(lparam);

    if (wparam == SIZE_MINIMIZED) {
      // Window is minimized, don't resize
      is_resizing_ = false;
      return 0;
    }

    if (wparam == SIZE_MAXIMIZED || wparam == SIZE_RESTORED) {
      // Window is maximized or restored, perform resize immediately
      OnResize(new_width, new_height);
      return 0;
    }

    // For SIZE_RESTORED during drag, store pending dimensions
    if (new_width > 0 && new_height > 0) {
      pending_width_ = new_width;
      pending_height_ = new_height;
      is_resizing_ = true;
    }
    return 0;
  }

  // Handle entering resize/move loop
  case WM_ENTERSIZEMOVE: {
    is_resizing_ = true;
    return 0;
  }

  // Handle exiting resize/move loop
  case WM_EXITSIZEMOVE: {
    is_resizing_ = false;
    // Perform the actual resize now that user has stopped dragging
    if (pending_width_ > 0 && pending_height_ > 0) {
      OnResize(pending_width_, pending_height_);
      pending_width_ = 0;
      pending_height_ = 0;
    }
    return 0;
  }

  // Any other messages send to the default message handler as our application
  // won't make use of them.
  default: {
    return DefWindowProc(hwnd, umsg, wparam, lparam);
  }
  }

  return 0;
}

void System::InitializeWindows(int screen_width, int screen_height) {
  WNDCLASSEX wc;
  DEVMODE dmScreenSettings;
  int posX, posY;

  // Get an external pointer to this object.
  g_system_instance = this;

  // Get the instance of this application.
  hinstance_ = GetModuleHandle(NULL);

  // Give the application a name.
  application_name_ = L"Engine";

  // Setup the windows class with default settings.
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wc.lpfnWndProc = WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hinstance_;
  wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
  wc.hIconSm = wc.hIcon;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = application_name_;
  wc.cbSize = sizeof(WNDCLASSEX);

  // Register the window class.
  RegisterClassEx(&wc);

  // Determine the resolution of the clients desktop screen.
  screen_width = GetSystemMetrics(SM_CXSCREEN);
  screen_height = GetSystemMetrics(SM_CYSCREEN);

  // Setup the screen settings depending on whether it is running in full screen
  // or in windowed mode.
  if (FULL_SCREEN) {
    // If full screen set the screen to maximum size of the users desktop and
    // 32bit.
    memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
    dmScreenSettings.dmSize = sizeof(dmScreenSettings);
    dmScreenSettings.dmPelsWidth = (unsigned long)screen_width;
    dmScreenSettings.dmPelsHeight = (unsigned long)screen_height;
    dmScreenSettings.dmBitsPerPel = 32;
    dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

    // Change the display settings to full screen.
    ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

    // Set the position of the window to the top left corner.
    posX = posY = 0;
  } else {
    // If windowed then set it to 800x600 resolution.
    screen_width = 800;
    screen_height = 600;

    // Place the window in the middle of the screen.
    posX = (GetSystemMetrics(SM_CXSCREEN) - screen_width) / 2;
    posY = (GetSystemMetrics(SM_CYSCREEN) - screen_height) / 2;
  }

  // Create the window with the screen settings and get the handle to it.
  // Use WS_OVERLAPPEDWINDOW for a standard resizable window with title bar
  DWORD windowStyle = FULL_SCREEN ? (WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP) 
                                   : WS_OVERLAPPEDWINDOW;

  // Adjust window size to account for borders and title bar
  if (!FULL_SCREEN) {
    RECT windowRect = {0, 0, screen_width, screen_height};
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    screen_width = windowRect.right - windowRect.left;
    screen_height = windowRect.bottom - windowRect.top;

    // Recalculate position after size adjustment
    posX = (GetSystemMetrics(SM_CXSCREEN) - screen_width) / 2;
    posY = (GetSystemMetrics(SM_CYSCREEN) - screen_height) / 2;
  }

  hwnd_ = CreateWindowEx(WS_EX_APPWINDOW, application_name_, application_name_,
                         windowStyle, posX, posY, screen_width, screen_height, 
                         NULL, NULL, hinstance_, NULL);

  // Bring the window up on the screen and set it as main focus.
  ShowWindow(hwnd_, SW_SHOW);
  SetForegroundWindow(hwnd_);
  SetFocus(hwnd_);

  // Only hide cursor in fullscreen mode
  if (FULL_SCREEN) {
    ShowCursor(false);
  }
}

void System::ShutdownWindows() {
  // Show the mouse cursor (only if it was hidden in fullscreen).
  if (FULL_SCREEN) {
    ShowCursor(true);
  }

  // Fix the display settings if leaving full screen mode.
  if (FULL_SCREEN) {
    ChangeDisplaySettings(NULL, 0);
  }

  // Remove the window.
  DestroyWindow(hwnd_);
  hwnd_ = nullptr;

  // Remove the application instance.
  UnregisterClass(application_name_, hinstance_);
  hinstance_ = nullptr;

  // Release the pointer to this class.
  g_system_instance = nullptr;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam,
                         LPARAM lparam) {
  switch (umessage) {
  // Check if the window is being destroyed.
  case WM_DESTROY: {
    PostQuitMessage(0);
    return 0;
  }

  // Check if the window is being closed.
  case WM_CLOSE: {
    PostQuitMessage(0);
    return 0;
  }

  // All other messages pass to the message handler in the system class.
  default: {
    return g_system_instance->MessageHandler(hwnd, umessage, wparam, lparam);
  }
  }
}