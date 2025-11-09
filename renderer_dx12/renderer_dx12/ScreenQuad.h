#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class DirectX12Device;

class ScreenQuadMaterial : public Effect::Material {
public:
  explicit ScreenQuadMaterial(std::shared_ptr<DirectX12Device> device);

  ScreenQuadMaterial(const ScreenQuadMaterial &rhs) = delete;

  ScreenQuadMaterial &operator=(const ScreenQuadMaterial &rhs) = delete;

  virtual ~ScreenQuadMaterial();

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 orthogonality_;
  };

public:
  virtual bool Initialize() override;

  bool UpdateConstantBuffer(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &orthogonality);

  ResourceSharedPtr GetConstantBuffer() const;

private:
  bool InitializeGraphicsPipelineState();

  bool InitializeRootSignature();

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr constant_buffer_ = nullptr;

  MatrixBufferType matrix_constant_data_ = {};
};

class ScreenQuad {
public:
  explicit ScreenQuad(std::shared_ptr<DirectX12Device> device);

  ScreenQuad(const ScreenQuad &rhs) = delete;

  ScreenQuad &operator=(const ScreenQuad &rhs) = delete;

  ~ScreenQuad() {}

private:
  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
  };

public:
  bool Initialize(UINT screen_width, UINT screen_height, UINT bitmap_width,
                  UINT bitmap_height);

  const VertexBufferView &GetVertexBufferView() const;

  const IndexBufferView &GetIndexBufferView() const;

  ScreenQuadMaterial *GetMaterial();

  bool UpdatePosition(int pos_x, int pos_y);

  const int GetIndexCount();

private:
  bool InitializeBuffers();

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ScreenQuadMaterial material_;

  ResourceSharedPtr vertex_buffer_ = nullptr;

  VertexBufferView vertex_buffer_view_;

  ResourceSharedPtr index_buffer_ = nullptr;

  IndexBufferView index_buffer_view_;

  UINT index_count_ = 0, vertex_count_ = 0;

  UINT screen_width_ = 0, screen_height_ = 0;

  UINT quad_height_ = 0, quad_width_ = 0;

  int last_pos_x_ = -1, last_pos_y_ = -1;
};
