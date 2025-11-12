#pragma once

#include <DirectXMath.h>
#include <memory>

#include "ScreenQuadMaterial.h"

class DirectX12Device;

class ScreenQuad {
public:
  ScreenQuad(std::shared_ptr<DirectX12Device> device,
             std::shared_ptr<ScreenQuadMaterial> material = nullptr);

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

  const VertexBufferView& GetVertexBufferView() const {
      return vertex_buffer_view_;
  }

  const IndexBufferView& GetIndexBufferView() const {
      return index_buffer_view_;
  }

  void SetVertexBufferView(const VertexBufferView& view) {
      vertex_buffer_view_ = view;
  }

  void SetIndexBufferView(const IndexBufferView &view);

  std::shared_ptr<ScreenQuadMaterial> GetMaterialShared() const;

  ScreenQuadMaterial* ScreenQuad::GetMaterial() { return material_.get(); }

  void SetMaterial(std::shared_ptr<ScreenQuadMaterial> material);

  bool UpdatePosition(int pos_x, int pos_y);

  const int GetIndexCount() const { return index_count_; }

private:
  bool InitializeBuffers();

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ScreenQuadMaterial> material_ = nullptr;

  ResourceSharedPtr vertex_buffer_ = nullptr;
  VertexBufferView vertex_buffer_view_;

  ResourceSharedPtr index_buffer_ = nullptr;
  IndexBufferView index_buffer_view_;

  UINT index_count_ = 0, vertex_count_ = 0;

  UINT screen_width_ = 0, screen_height_ = 0;

  UINT quad_height_ = 0, quad_width_ = 0;

  int last_pos_x_ = -1, last_pos_y_ = -1;
};
