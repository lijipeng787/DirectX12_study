#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Scene.h"
#include "Model.h"
#include "Text.h"

class DirectX12Device;

namespace ResourceLoader {
class ShaderLoader;
}

// Scene that renders Model and Text to the default offscreen texture
// This provides content for OffscreenPreviewScene to display
class OffscreenRenderScene {
public:
  OffscreenRenderScene() = default;

  OffscreenRenderScene(const OffscreenRenderScene &rhs) = delete;

  auto operator=(const OffscreenRenderScene &rhs)
      -> OffscreenRenderScene & = delete;

  ~OffscreenRenderScene() = default;

  // Scene interface
  auto Initialize(const SceneInitializeContext &ctx) -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  // RenderReflection is used to render to offscreen texture before main pass
  auto RenderReflection(const SceneReflectionContext &ctx) -> bool;

  // Regular Render does nothing (content already rendered in RenderReflection)
  auto Render(const SceneRenderContext &ctx) -> bool { return true; }

  void SetRotationAngle(float radians) { rotation_angle_ = radians; }

  // Set shared text object (to render same text to offscreen as main screen)
  void SetSharedText(std::shared_ptr<Text> text) { text_ = text; }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<Model> model_ = nullptr;

  std::shared_ptr<Text> text_ = nullptr;

  DirectX::XMMATRIX base_view_matrix_ = DirectX::XMMatrixIdentity();

  float rotation_angle_ = 0.0f;

  // Cached rendering resources (avoid repeated lookups)
  struct CachedResources {
    ID3D12RootSignature *light_root_signature = nullptr;
    ID3D12PipelineState *light_pso = nullptr;
    ID3D12Resource *light_matrix_cb = nullptr;
    ID3D12Resource *light_cb = nullptr;
    ID3D12Resource *fog_cb = nullptr;

    ID3D12RootSignature *font_root_signature = nullptr;
    ID3D12PipelineState *font_pso = nullptr;
    ID3D12Resource *font_matrix_cb = nullptr;
    ID3D12Resource *font_pixel_cb = nullptr;
  } cached_resources_;
};

