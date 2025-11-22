#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <memory>
#include <vector>

#include "Scene.h"
#include "ShaderLoader.h"

namespace Lighting {
class LightManager;
}

constexpr bool FULL_SCREEN = false;
constexpr bool VSYNC_ENABLED = true;
constexpr float SCREEN_DEPTH = 1000.0f;
constexpr float SCREEN_NEAR = 0.1f;

class DirectX12Device;
class Camera;
class Input;
class Fps;
class CPUUsageTracker;
class Text;  // Still needed for external SetFps/SetCpu interface

class Graphics {
public:
  Graphics() = default;

  Graphics(const Graphics &rhs) = delete;

  auto operator=(const Graphics &rhs) -> Graphics = delete;

  ~Graphics() {}

  auto Initialize(int, int, HWND) -> bool;

  void Shutdown();

  auto Frame(float delta_seconds, Input *input) -> bool;

private:
  auto Render() -> bool;

  void UpdateCameraFromInput(float delta_seconds, Input *input);

  // Helper methods for better organization
  auto InitializeShaders(HWND hwnd) -> bool;

  auto InitializeRenderObjects(int screenWidth, int screenHeight, HWND hwnd) -> bool;
  
  auto InitializeScenes(HWND hwnd) -> bool;



  std::shared_ptr<DirectX12Device> d3d12_device_ = nullptr;

  std::shared_ptr<Lighting::LightManager> light_manager_ = nullptr;

  std::shared_ptr<Camera> camera_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<Fps> fps_ = nullptr;

  std::shared_ptr<CPUUsageTracker> cpu_usage_tracker_ = nullptr;

  // Unified scene container using type erasure
  std::vector<Scene> scenes_;

  // Cached references for external interaction
  std::shared_ptr<Text> text_cache_ = nullptr;  // For SetFps/SetCpu calls

  // Camera control
  float camera_move_speed_ = 5.0f;
  float camera_turn_speed_ = 90.0f;

  // Shared rotation for all rotating scenes
  float shared_rotation_angle_ = 0.0f;
  float shared_rotation_speed_ = DirectX::XM_PI * 0.25f;
};
