#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

class PBRMaterial : public Effect::Material {
public:
  explicit PBRMaterial(std::shared_ptr<DirectX12Device> device);

  PBRMaterial(const PBRMaterial &rhs) = delete;

  auto operator=(const PBRMaterial &rhs) -> PBRMaterial & = delete;

  ~PBRMaterial();

  auto Initialize() -> bool override;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateCameraConstant(const DirectX::XMFLOAT3 &camera_position) -> bool;

  auto UpdateLightConstant(const DirectX::XMFLOAT3 &light_direction) -> bool;

  // New unified light interface - extract parameters from SceneLight
  auto UpdateFromLight(const Lighting::SceneLight *scene_light) -> bool;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr;

  auto GetCameraConstantBuffer() const -> ResourceSharedPtr;

  auto GetLightConstantBuffer() const -> ResourceSharedPtr;

private:
  auto InitializeRootSignature() -> bool;

  auto InitializeGraphicsPipelineState() -> bool;

  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
    DirectX::XMFLOAT4X4 normal_;
  };

  struct CameraBufferType {
    DirectX::XMFLOAT4 camera_position_;
  };

  struct LightBufferType {
    DirectX::XMFLOAT4 light_direction_;
  };

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  MatrixBufferType matrix_constant_data_ = {};

  ResourceSharedPtr camera_constant_buffer_ = nullptr;
  CameraBufferType camera_constant_data_ = {};

  ResourceSharedPtr light_constant_buffer_ = nullptr;
  LightBufferType light_constant_data_ = {};
};
