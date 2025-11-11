#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

class ReflectionFloorMaterial : public Effect::Material {
public:
  explicit ReflectionFloorMaterial(std::shared_ptr<DirectX12Device> device);

  ReflectionFloorMaterial(const ReflectionFloorMaterial &rhs) = delete;
  ReflectionFloorMaterial &
  operator=(const ReflectionFloorMaterial &rhs) = delete;

  ~ReflectionFloorMaterial() override = default;

  bool Initialize() override;

  ResourceSharedPtr GetMatrixConstantBuffer() const;

  ResourceSharedPtr GetReflectionConstantBuffer() const;

  bool UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection);

  bool UpdateReflectionConstant(const DirectX::XMMATRIX &reflection);

private:
  bool InitializeRootSignature();

  bool InitializeGraphicsPipelineState();

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
  };

  struct ReflectionBufferType {
    DirectX::XMFLOAT4X4 reflection;
  };

  std::shared_ptr<DirectX12Device> device_ = nullptr;
  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  ResourceSharedPtr reflection_constant_buffer_ = nullptr;
  MatrixBufferType matrix_data_ = {};
  ReflectionBufferType reflection_data_ = {};
};

