#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <DirectXMath.h>
#include <memory>

#include "Material.h"

class BitmapMaterial : public Effect::Material {
public:
  BitmapMaterial();

  BitmapMaterial(const BitmapMaterial &rhs) = delete;

  BitmapMaterial &operator=(const BitmapMaterial &rhs) = delete;

  virtual ~BitmapMaterial();

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
  ResourceSharedPtr constant_buffer_ = nullptr;

  MatrixBufferType matrix_constant_data_ = {};
};

class Bitmap {
public:
  Bitmap() {}

  Bitmap(const Bitmap &rhs) = delete;

  Bitmap &operator=(const Bitmap &rhs) = delete;

  ~Bitmap() {}

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

  BitmapMaterial *GetMaterial();

  bool UpdateBitmapPosition(int pos_x, int pos_y);

  const int GetIndexCount();

private:
  bool InitializeBuffers();

private:
  BitmapMaterial material_ = {};

  ResourceSharedPtr vertex_buffer_ = nullptr;

  VertexBufferView vertex_buffer_view_;

  ResourceSharedPtr index_buffer_ = nullptr;

  IndexBufferView index_buffer_view_;

  UINT index_count_ = 0, vertex_count_ = 0;

  UINT screen_width_ = 0, screen_height_ = 0;

  UINT bitmap_height_ = 0, bitmap_width_ = 0;

  int previous_pos_x_ = -1, previous_pos_y_ = -1;
};

#endif // !_BITMAP_H_