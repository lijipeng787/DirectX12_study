#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Scene.h"
#include "Text.h"

class DirectX12Device;

namespace ResourceLoader {
class ShaderLoader;
}

// Scene wrapper for Text rendering
// Renders FPS and CPU usage text overlays
class TextOverlayScene {
public:
  TextOverlayScene() = default;

  TextOverlayScene(const TextOverlayScene &rhs) = delete;

  auto operator=(const TextOverlayScene &rhs) -> TextOverlayScene & = delete;

  ~TextOverlayScene() = default;

  // Scene interface
  auto Initialize(const SceneInitializeContext &ctx) -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  auto Render(const SceneRenderContext &ctx) -> bool;

  void SetRotationAngle(float /*radians*/) {
    // Text doesn't rotate
  }

  // Text-specific interface
  auto GetText() -> std::shared_ptr<Text> { return text_; }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<Text> text_ = nullptr;

  DirectX::XMMATRIX base_view_matrix_ = DirectX::XMMatrixIdentity();

  int screen_width_ = 0;
  int screen_height_ = 0;
};

