#pragma once

#include <DirectXMath.h>
#include <memory>

#include "SpecularMapModel.h"

class Camera;
class DirectX12Device;

namespace Lighting {
class LightManager;
class SceneLight;
} // namespace Lighting

namespace ResourceLoader {
class ShaderLoader;
}

class SpecularMappingScene {
public:
  SpecularMappingScene(std::shared_ptr<DirectX12Device> device,
                       std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader,
                       std::shared_ptr<Lighting::LightManager> light_manager,
                       std::shared_ptr<Camera> camera);

  SpecularMappingScene(const SpecularMappingScene &rhs) = delete;
  
  auto operator=(const SpecularMappingScene &rhs) -> SpecularMappingScene &
      = delete;

  ~SpecularMappingScene() = default;

  auto Initialize() -> bool;

  void Shutdown();

  void Update(float delta_seconds);

  auto Render(const DirectX::XMMATRIX &view, const DirectX::XMMATRIX &projection,
              const Lighting::SceneLight *scene_light) -> bool;

  void SetRotationAngle(float radians);

private:
  auto EnsureShadersLoaded() -> bool;

private:
  std::shared_ptr<DirectX12Device> device_;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_;
  
  std::shared_ptr<Lighting::LightManager> light_manager_;
  
  std::shared_ptr<Camera> camera_;

  std::shared_ptr<SpecularMapModel> model_ = nullptr;

  bool shaders_loaded_ = false;

  float rotation_radians_ = 0.0f;

  DirectX::XMFLOAT3 position_ = {6.0f, 1.5f, 6.0f};
};

