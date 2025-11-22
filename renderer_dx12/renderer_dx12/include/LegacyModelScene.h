#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Model.h"
#include "Scene.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

namespace ResourceLoader {
class ShaderLoader;
}

// Scene wrapper for legacy Model class
// Renders a textured cube with Blinn-Phong lighting and fog
class LegacyModelScene {
public:
  LegacyModelScene() = default;

  LegacyModelScene(const LegacyModelScene &rhs) = delete;

  auto operator=(const LegacyModelScene &rhs) -> LegacyModelScene & = delete;

  ~LegacyModelScene() = default;

  // Scene interface
  auto Initialize(const SceneInitializeContext &ctx) -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  auto Render(const SceneRenderContext &ctx) -> bool;

  void SetRotationAngle(float radians);

  // Legacy-specific interface
  auto GetModel() -> std::shared_ptr<Model> { return model_; }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<Model> model_ = nullptr;

  float rotation_radians_ = 0.0f;

  // Position configuration
  static constexpr DirectX::XMFLOAT3 kModelPosition = {-6.0f, 1.5f, -6.0f};
};

