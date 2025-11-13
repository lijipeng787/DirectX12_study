#pragma once

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <memory>
#include "Timer.h"
#include "Input.h"
#include "Graphics.h"

class System {

public:
  System() {}

  System(const System &rhs) = delete;

  System &operator=(const System &rhs) = delete;

  ~System() {}

public:
  bool Initialize();

  void Shutdown();

  void Run();

  LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
  bool Frame();

  void InitializeWindows(int screen_width, int screen_height);

  void ShutdownWindows();

private:
  LPCWSTR application_name_;

  HINSTANCE hinstance_;

  HWND hwnd_;

  std::unique_ptr<Input> input_ = nullptr;

  std::unique_ptr<Graphics> graphics_ = nullptr;

  std::unique_ptr<TimerClass> timer_ = nullptr;
};

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

static System *g_system_instance = 0;
