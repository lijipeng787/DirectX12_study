#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
#include "Material.h"

class DirectX12Device;

class ReflectionFloorMaterial : public Effect::Material {
public:
  explicit ReflectionFloorMaterial(std::shared_ptr<DirectX12Device> device);

  ReflectionFloorMaterial(const ReflectionFloorMaterial &rhs) = delete;
  
  auto operator=(const ReflectionFloorMaterial &rhs) -> ReflectionFloorMaterial & = delete;

  ~ReflectionFloorMaterial() override = default;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr;

  auto GetReflectionConstantBuffer() const -> ResourceSharedPtr;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateReflectionConstant(const DirectX::XMMATRIX &reflection) -> bool;

  auto InitializeRootSignature() -> bool;

  auto InitializeGraphicsPipelineState() -> bool;

  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
  };

  struct ReflectionBufferType {
    DirectX::XMFLOAT4X4 reflection_;
  };

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ConstantBuffer<MatrixBufferType> matrix_constant_buffer_;
  ConstantBuffer<ReflectionBufferType> reflection_constant_buffer_;
  
  MatrixBufferType matrix_constant_data_ = {};
  ReflectionBufferType reflection_constant_data_ = {};
};

