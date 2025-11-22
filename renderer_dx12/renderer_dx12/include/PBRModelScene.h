#pragma once

#include <DirectXMath.h>
#include <memory>

#include "PBRModel.h"
#include "Scene.h"

class Camera;
class DirectX12Device;

namespace Lighting {
class SceneLight;
}

namespace ResourceLoader {
class ShaderLoader;
}

// Scene wrapper for PBRModel
// Renders a physically-based rendered sphere
class PBRModelScene {
public:
  PBRModelScene() = default;

  PBRModelScene(const PBRModelScene &rhs) = delete;

  auto operator=(const PBRModelScene &rhs) -> PBRModelScene & = delete;

  ~PBRModelScene() = default;

  // Scene interface
  auto Initialize(const SceneInitializeContext &ctx) -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  auto Render(const SceneRenderContext &ctx) -> bool;

  void SetRotationAngle(float radians);

  // PBR-specific interface
  auto GetModel() -> std::shared_ptr<PBRModel> { return model_; }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<Camera> camera_ = nullptr;

  std::shared_ptr<PBRModel> model_ = nullptr;

  float rotation_radians_ = 0.0f;

  // Position configuration
  static constexpr DirectX::XMFLOAT3 kModelPosition = {6.0f, 1.5f, -6.0f};
};

