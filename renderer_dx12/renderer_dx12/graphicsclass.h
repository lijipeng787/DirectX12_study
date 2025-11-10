#pragma once

#include <DirectXMath.h>
#include <Windows.h>
#include <memory>

#include "ShaderLoader.h"

namespace Lighting {
class LightManager;
}

constexpr bool FULL_SCREEN = false;
constexpr bool VSYNC_ENABLED = true;
constexpr float SCREEN_DEPTH = 1000.0f;
constexpr float SCREEN_NEAR = 0.1f;

class DirectX12Device;
class Light;
class Camera;
class Input;
class ScreenQuad;
class Text;
class Model;
class PBRModel;
class Fps;
class CPUUsageTracker;

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

  std::shared_ptr<DirectX12Device> d3d12_device_ = nullptr;

  std::shared_ptr<Lighting::LightManager> light_manager_ = nullptr;

  std::shared_ptr<Camera> camera_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<ScreenQuad> bitmap_ = nullptr;

  std::shared_ptr<Text> text_ = nullptr;

  std::shared_ptr<Model> model_ = nullptr;

  std::shared_ptr<PBRModel> pbr_model_ = nullptr;

  std::shared_ptr<Fps> fps_ = nullptr;

  std::shared_ptr<CPUUsageTracker> cpu_usage_tracker_ = nullptr;

  float camera_move_speed_ = 5.0f;
};
