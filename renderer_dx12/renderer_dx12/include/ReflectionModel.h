#pragma once

#include <DirectXMath.h>
#include <memory>

#include "TextureLoader.h"
#include "ResourceManager.h"

class DirectX12Device;

class ReflectionModel {
public:
  explicit ReflectionModel(std::shared_ptr<DirectX12Device> device);

  ReflectionModel(const ReflectionModel &rhs) = delete;

  auto operator=(const ReflectionModel &rhs) -> ReflectionModel & = delete;

  ~ReflectionModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                  unsigned int texture_count) -> bool;

  auto GetIndexCount() const -> UINT { return model_resource_ ? model_resource_->index_count : 0; }

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const {
    static D3D12_VERTEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->vertex_buffer_view : null_view;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const {
    static D3D12_INDEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->index_buffer_view : null_view;
  }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

  auto GetTextureResource(size_t index) const -> ResourceSharedPtr;

private:
  auto LoadTextures(WCHAR **texture_filename_arr, unsigned int texture_count) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  std::shared_ptr<ModelResource> model_resource_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_loader_ = nullptr;
};
