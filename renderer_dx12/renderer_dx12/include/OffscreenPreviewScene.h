#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Scene.h"
#include "ScreenQuad.h"

class DirectX12Device;

namespace ResourceLoader {
class ShaderLoader;
}

// Scene that renders an offscreen texture preview as a bitmap
// Displays the default offscreen render target in the corner of the screen
class OffscreenPreviewScene {
public:
  OffscreenPreviewScene() = default;

  OffscreenPreviewScene(const OffscreenPreviewScene &rhs) = delete;

  auto operator=(const OffscreenPreviewScene &rhs)
      -> OffscreenPreviewScene & = delete;

  ~OffscreenPreviewScene() = default;

  // Scene interface
  auto Initialize(const SceneInitializeContext &ctx) -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  auto Render(const SceneRenderContext &ctx) -> bool;

  void SetRotationAngle(float /*radians*/) {
    // Preview doesn't rotate
  }

  // Preview-specific configuration
  void SetPosition(int x, int y) {
    position_x_ = x;
    position_y_ = y;
  }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<ScreenQuad> bitmap_ = nullptr;

  DirectX::XMMATRIX base_view_matrix_ = DirectX::XMMatrixIdentity();

  int position_x_ = 100;
  int position_y_ = 100;
};

