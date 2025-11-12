#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ConstantBuffer.h"
#include "Material.h"

class DirectX12Device;

class TextMaterial : public Effect::Material {
public:
  explicit TextMaterial(std::shared_ptr<DirectX12Device> device)
      : device_(std::move(device)) {}

  TextMaterial(const TextMaterial &rhs) = delete;
  auto operator=(const TextMaterial &rhs) -> TextMaterial & = delete;

  ~TextMaterial() override;

  auto Initialize() -> bool override;

  auto GetMatrixConstantBuffer() const -> ResourceSharedPtr {
    return matrix_constant_buffer_.GetResource();
  }

  auto GetPixelConstantBuffer() const -> ResourceSharedPtr {
    return pixel_color_constant_buffer_.GetResource();
  }

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &base_view,
                            const DirectX::XMMATRIX &orthonality) -> bool;

  auto UpdateLightConstant(const DirectX::XMFLOAT4 &pixel_color) -> bool;

private:
  struct ConstantBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 base_view_;
    DirectX::XMFLOAT4X4 orthonality_;
  };

  struct PixelBufferType {
    DirectX::XMFLOAT4 pixel_color_;
  };

  auto InitializeConstantBuffer() -> bool;
  auto InitializeRootSignature() -> bool;
  auto InitializeGraphicsPipelineState() -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ConstantBuffer<ConstantBufferType> matrix_constant_buffer_;
  ConstantBufferType matrix_constant_data_ = {};

  ConstantBuffer<PixelBufferType> pixel_color_constant_buffer_;
  PixelBufferType pixel_color_constant_data_ = {};
};


