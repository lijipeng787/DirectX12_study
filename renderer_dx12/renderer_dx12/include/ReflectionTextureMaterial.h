#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
#include "Material.h"

class DirectX12Device;

class ReflectionTextureMaterial : public Effect::Material {
public:
  explicit ReflectionTextureMaterial(std::shared_ptr<DirectX12Device> device);

  ReflectionTextureMaterial(const ReflectionTextureMaterial &rhs) = delete;

  auto operator=(const ReflectionTextureMaterial &rhs) -> ReflectionTextureMaterial & = delete;

  ~ReflectionTextureMaterial() override = default;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto InitializeRootSignature() -> bool;

  auto InitializeGraphicsPipelineState() -> bool;

  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
  };

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ConstantBuffer<MatrixBufferType> matrix_constant_buffer_;
  MatrixBufferType matrix_constant_data_ = {};
};

