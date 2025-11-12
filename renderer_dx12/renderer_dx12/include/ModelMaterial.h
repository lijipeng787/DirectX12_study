#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
#include "Material.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

class ModelMaterial : public Effect::Material {
public:
  explicit ModelMaterial(std::shared_ptr<DirectX12Device> device)
      : device_(std::move(device)) {}

  ModelMaterial(const ModelMaterial &rhs) = delete;
  auto operator=(const ModelMaterial &rhs) -> ModelMaterial & = delete;

  ~ModelMaterial() override = default;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr {
    return matrix_constant_buffer_.GetResource();
  }

  auto GetLightConstantBuffer() const -> ResourceSharedPtr {
    return light_constant_buffer_.GetResource();
  }

  auto GetFogConstantBuffer() const -> ResourceSharedPtr {
    return fog_constant_buffer_.GetResource();
  }

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateLightConstant(const DirectX::XMFLOAT4 &ambient_color,
                           const DirectX::XMFLOAT4 &diffuse_color,
                           const DirectX::XMFLOAT3 &direction) -> bool;

  auto UpdateFromLight(const Lighting::SceneLight *scene_light) -> bool;

  auto UpdateFogConstant(float fog_begin, float fog_end) -> bool;

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
    DirectX::XMFLOAT4X4 normal_;
  };

  struct LightType {
    DirectX::XMFLOAT4 ambient_color_;
    DirectX::XMFLOAT4 diffuse_color_;
    DirectX::XMFLOAT3 direction_;
    float padding_;
  };

  struct FogBufferType {
    float fog_start_;
    float fog_end_;
    float padding1_;
    float padding2_;
  };

  auto InitializeRootSignature() -> bool;

  auto InitializeGraphicsPipelineState() -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ConstantBuffer<MatrixBufferType> matrix_constant_buffer_ = {};
  MatrixBufferType matrix_constant_data_ = {};

  ConstantBuffer<LightType> light_constant_buffer_ = {};
  LightType light_constant_data_ = {};

  ConstantBuffer<FogBufferType> fog_constant_buffer_ = {};
  FogBufferType fog_constant_data_ = {};
};


