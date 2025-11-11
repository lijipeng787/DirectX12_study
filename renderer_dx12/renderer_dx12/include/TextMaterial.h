#pragma once

#include <DirectXMath.h>
#include <memory>

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
    return matrix_constant_buffer_;
  }

  auto GetPixelConstantBuffer() const -> ResourceSharedPtr {
    return pixel_color_constant_buffer_;
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

private:
  auto InitializeConstantBuffer() -> bool;
  auto InitializeRootSignature() -> bool;
  auto InitializeGraphicsPipelineState() -> bool;

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  ConstantBufferType matrix_constant_data_ = {};

  ResourceSharedPtr pixel_color_constant_buffer_ = nullptr;
  PixelBufferType pixel_color_constant_data_ = {};
};


