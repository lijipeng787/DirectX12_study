#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
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

  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
  };

  struct CameraBufferType {
    DirectX::XMFLOAT3 camera_position_;
    float padding_;
  };

  struct LightBufferType {
    DirectX::XMFLOAT4 diffuse_color_;
    DirectX::XMFLOAT4 specular_color_;
    DirectX::XMFLOAT3 light_direction_;
    float specular_power_;
  };

  std::shared_ptr<DirectX12Device> device_;

  ConstantBuffer<MatrixBufferType> matrix_constant_buffer_;
  MatrixBufferType matrix_constant_data_ = {};

  ConstantBuffer<CameraBufferType> camera_constant_buffer_;
  CameraBufferType camera_constant_data_ = {};

  ConstantBuffer<LightBufferType> light_constant_buffer_;
  LightBufferType light_constant_data_ = {};
};

