#pragma once

#include <DirectXMath.h>
#include <memory>

#include "TextureLoader.h"

class DirectX12Device;

class ReflectionModel {
public:
  explicit ReflectionModel(std::shared_ptr<DirectX12Device> device);

  ReflectionModel(const ReflectionModel &rhs) = delete;

  auto operator=(const ReflectionModel &rhs) -> ReflectionModel & = delete;

  ~ReflectionModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                  unsigned int texture_count) -> bool;

  auto GetIndexCount() const -> UINT { return index_count_; }

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const {
    return vertex_buffer_view_;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const {
    return index_buffer_view_;
  }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

  auto GetTextureResource(size_t index) const -> ResourceSharedPtr;

private:
  struct VertexType {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texcoord;
    DirectX::XMFLOAT3 normal;
  };

  struct ModelType {
    float x, y, z;
    float tu, tv;
    float nx, ny, nz;
  };

  auto LoadModel(WCHAR *filename) -> bool;

  auto InitializeBuffers() -> bool;

  auto LoadTextures(WCHAR **texture_filename_arr, unsigned int texture_count) -> bool;

  void ReleaseModel();

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr vertex_buffer_ = nullptr;
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

  ResourceSharedPtr index_buffer_ = nullptr;
  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT vertex_count_ = 0;
  
  UINT index_count_ = 0;

  ModelType *model_data_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_loader_ = nullptr;
};

