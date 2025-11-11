#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

class SpecularMapMaterial : public Effect::Material {
public:
  explicit SpecularMapMaterial(std::shared_ptr<DirectX12Device> device);

  SpecularMapMaterial(const SpecularMapMaterial &rhs) = delete;
  auto operator=(const SpecularMapMaterial &rhs) -> SpecularMapMaterial & =
      delete;

  ~SpecularMapMaterial() override = default;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr;
  auto GetCameraConstantBuffer() const -> ResourceSharedPtr;
  auto GetLightConstantBuffer() const -> ResourceSharedPtr;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateCameraConstant(const DirectX::XMFLOAT3 &camera_position) -> bool;

  auto UpdateLightFromScene(const Lighting::SceneLight *scene_light) -> bool;
  auto UpdateLightConstant(const DirectX::XMFLOAT4 &diffuse_color,
                           const DirectX::XMFLOAT4 &specular_color,
                           const DirectX::XMFLOAT3 &light_direction,
                           float specular_power) -> bool;

private:
  auto InitializeRootSignature() -> bool;
  auto InitializeGraphicsPipelineState() -> bool;

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
  };

  struct CameraBufferType {
    DirectX::XMFLOAT3 camera_position;
    float padding;
  };

  struct LightBufferType {
    DirectX::XMFLOAT4 diffuse_color;
    DirectX::XMFLOAT4 specular_color;
    DirectX::XMFLOAT3 light_direction;
    float specular_power;
  };

  std::shared_ptr<DirectX12Device> device_;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  MatrixBufferType matrix_data_ = {};

  ResourceSharedPtr camera_constant_buffer_ = nullptr;
  CameraBufferType camera_data_ = {};

  ResourceSharedPtr light_constant_buffer_ = nullptr;
  LightBufferType light_data_ = {};
};

