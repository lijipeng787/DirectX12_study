#pragma once

#ifndef _PBRMATERIAL_H_
#define _PBRMATERIAL_H_

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

class PBRMaterial : public Effect::Material {
public:
  explicit PBRMaterial(std::shared_ptr<DirectX12Device> device);

  PBRMaterial(const PBRMaterial &rhs) = delete;
  PBRMaterial &operator=(const PBRMaterial &rhs) = delete;

  ~PBRMaterial();

public:
  bool Initialize() override;

  bool UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection);

  bool UpdateCameraConstant(const DirectX::XMFLOAT3 &camera_position);

  bool UpdateLightConstant(const DirectX::XMFLOAT3 &light_direction);

  ResourceSharedPtr GetMatrixConstantBuffer() const;

  ResourceSharedPtr GetCameraConstantBuffer() const;

  ResourceSharedPtr GetLightConstantBuffer() const;

private:
  bool InitializeRootSignature();

  bool InitializeGraphicsPipelineState();

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT4X4 normalMatrix; // Inverse-transpose of world matrix for
                                      // correct normal transformation
  };

  struct CameraBufferType {
    DirectX::XMFLOAT4 camera_position;
  };

  struct LightBufferType {
    DirectX::XMFLOAT4 light_direction;
  };

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;
  MatrixBufferType matrix_data_ = {};

  ResourceSharedPtr camera_constant_buffer_ = nullptr;
  CameraBufferType camera_data_ = {};

  ResourceSharedPtr light_constant_buffer_ = nullptr;
  LightBufferType light_data_ = {};
};

#endif // !_PBRMATERIAL_H_
