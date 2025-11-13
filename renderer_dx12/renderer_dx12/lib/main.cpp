#include "stdafx.h"
#include "System.h"

#include <cstdio>
#include <iostream>

namespace {

bool InitializeConsole() {
#if defined(_DEBUG)
  const wchar_t *console_title = L"renderer_dx12 Log";
#else
  const wchar_t *console_title = L"renderer_dx12 Console";
#endif

  if (!AllocConsole()) {
    return false;
  }

  FILE *stream = nullptr;
  freopen_s(&stream, "CONOUT$", "w", stdout);
  freopen_s(&stream, "CONOUT$", "w", stderr);
  freopen_s(&stream, "CONIN$", "r", stdin);

  SetConsoleTitleW(console_title);

  std::ios::sync_with_stdio();

  std::wcout << L"[renderer_dx12] Console initialized." << std::endl;
  return true;
}

void ShutdownConsole() {
  std::wcout << L"[renderer_dx12] Console shutting down." << std::endl;
  FreeConsole();
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline,
                   int iCmdshow) {

  const bool console_initialized = InitializeConsole();

  auto system = new System();
  if (!system) {
    if (console_initialized) {
      ShutdownConsole();
    }
    return 0;
  }

  auto result = system->Initialize();
  if (result) {
    system->Run();
  }

  system->Shutdown();
  delete system;
  system = 0;

  if (console_initialized) {
    ShutdownConsole();
  }

  return 0;
}