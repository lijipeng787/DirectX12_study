#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

class BumpMapMaterial : public Effect::Material {
public:
  explicit BumpMapMaterial(std::shared_ptr<DirectX12Device> device);

  BumpMapMaterial(const BumpMapMaterial &rhs) = delete;
  auto operator=(const BumpMapMaterial &rhs) -> BumpMapMaterial & = delete;

  ~BumpMapMaterial() override = default;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr;
  auto GetLightConstantBuffer() const -> ResourceSharedPtr;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateLightFromScene(const Lighting::SceneLight *scene_light) -> bool;

  auto UpdateLightConstant(const DirectX::XMFLOAT4 &diffuse_color,
                           const DirectX::XMFLOAT3 &light_direction) -> bool;

private:
  auto InitializeRootSignature() -> bool;
  auto InitializeGraphicsPipelineState() -> bool;

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
  };

  struct LightBufferType {
    DirectX::XMFLOAT4 diffuse_color;
    DirectX::XMFLOAT3 light_direction;
    float padding;
  };

  std::shared_ptr<DirectX12Device> device_;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  MatrixBufferType matrix_data_ = {};

  ResourceSharedPtr light_constant_buffer_ = nullptr;
  LightBufferType light_data_ = {};
};

