#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

class ScreenQuadMaterial : public Effect::Material {
public:
  explicit ScreenQuadMaterial(std::shared_ptr<DirectX12Device> device)
      : device_(std::move(device)) {}

  ScreenQuadMaterial(const ScreenQuadMaterial &rhs) = delete;
  auto operator=(const ScreenQuadMaterial &rhs) -> ScreenQuadMaterial & = delete;

  ~ScreenQuadMaterial() override = default;

  auto Initialize() -> bool override;

  auto UpdateConstantBuffer(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &orthogonality) -> bool;

  auto GetConstantBuffer() const -> ResourceSharedPtr;

  auto IsInitialized() const -> bool { return initialized_; }

  void SetExternalConstantBuffer(
      const ResourceSharedPtr &constant_buffer,
      const DirectX::XMMATRIX &initial_world = DirectX::XMMatrixIdentity(),
      const DirectX::XMMATRIX &initial_view = DirectX::XMMatrixIdentity(),
      const DirectX::XMMATRIX &initial_ortho = DirectX::XMMatrixIdentity());

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 orthogonality_;
  };

private:
  auto InitializeGraphicsPipelineState() -> bool;
  auto InitializeRootSignature() -> bool;

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr constant_buffer_ = nullptr;
  MatrixBufferType matrix_constant_data_ = {};

  bool initialized_ = false;
};


