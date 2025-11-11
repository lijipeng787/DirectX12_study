#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ReflectionFloorMaterial.h"
#include "ReflectionModel.h"
#include "ReflectionTextureMaterial.h"
#include "RenderTexture.h"

class Camera;
class DirectX12Device;

namespace Lighting {
class SceneLight;
} // namespace Lighting

namespace ResourceLoader {
class ShaderLoader;
} // namespace ResourceLoader

class ReflectionScene {
public:
  ReflectionScene(std::shared_ptr<DirectX12Device> device,
                  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader,
                  std::shared_ptr<Camera> camera);

  ReflectionScene(const ReflectionScene &rhs) = delete;

  auto operator=(const ReflectionScene &rhs) -> ReflectionScene & = delete;

  ~ReflectionScene() = default;

  auto Initialize() -> bool;

  auto Shutdown() -> void;

  auto Update(float delta_seconds) -> void;

  auto RenderReflectionMap(const DirectX::XMMATRIX &projection) -> bool;

  auto Render(const DirectX::XMMATRIX &view, const DirectX::XMMATRIX &projection) -> bool;

  auto SetRotationAngle(float radians) -> void { rotation_radians_ = radians; }

private:
  auto EnsureShadersLoaded() -> bool;

  auto RenderReflectionTexture(const DirectX::XMMATRIX &projection) -> bool;

  auto BuildFloorDescriptorHeap() -> bool;

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;
  
  std::shared_ptr<Camera> camera_ = nullptr;

  std::unique_ptr<RenderTexture> render_texture_ = nullptr;
  
  std::shared_ptr<ReflectionModel> cube_model_ = nullptr;
  
  std::shared_ptr<ReflectionModel> floor_model_ = nullptr;
  
  std::shared_ptr<ReflectionTextureMaterial> cube_material_ = nullptr;
  
  std::shared_ptr<ReflectionFloorMaterial> floor_material_ = nullptr;

  DescriptorHeapPtr floor_descriptor_heap_ = nullptr;

  bool shaders_loaded_ = false;

  float rotation_radians_ = 0.0f;
  
  float rotation_speed_ = DirectX::XM_PI * 0.25f;
  
  float reflection_plane_height_ = -1.5f;
  
  DirectX::XMFLOAT3 cube_position_ = {0.0f, 2.0f, 0.0f};
  
  float floor_scale_ = 3.0f;
};

