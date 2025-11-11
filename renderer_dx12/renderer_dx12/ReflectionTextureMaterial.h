#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

class ReflectionTextureMaterial : public Effect::Material {
public:
  explicit ReflectionTextureMaterial(std::shared_ptr<DirectX12Device> device);

  ReflectionTextureMaterial(const ReflectionTextureMaterial &rhs) = delete;
  ReflectionTextureMaterial &
  operator=(const ReflectionTextureMaterial &rhs) = delete;

  ~ReflectionTextureMaterial() override = default;

  bool Initialize() override;

  ResourceSharedPtr GetMatrixConstantBuffer() const;

  bool UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection);

private:
  bool InitializeRootSignature();

  bool InitializeGraphicsPipelineState();

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
  };

  std::shared_ptr<DirectX12Device> device_ = nullptr;
  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  MatrixBufferType matrix_data_ = {};
};

