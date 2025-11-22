#pragma once

#include <DirectXMath.h>
#include <memory>

#include "BumpMapMaterial.h"
#include "TextureLoader.h"
#include "ResourceManager.h"

class DirectX12Device;

class BumpMapModel {
public:
  explicit BumpMapModel(std::shared_ptr<DirectX12Device> device);

  BumpMapModel(const BumpMapModel &rhs) = delete;

  auto operator=(const BumpMapModel &rhs) -> BumpMapModel & = delete;

  ~BumpMapModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                  unsigned int texture_count) -> bool;

  auto GetIndexCount() const -> UINT { return model_resource_ ? model_resource_->index_count : 0; }

  auto GetVertexBufferView() const -> const D3D12_VERTEX_BUFFER_VIEW & {
    static D3D12_VERTEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->vertex_buffer_view : null_view;
  }

  auto GetIndexBufferView() const -> const D3D12_INDEX_BUFFER_VIEW & {
    static D3D12_INDEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->index_buffer_view : null_view;
  }

  auto GetMaterial() -> BumpMapMaterial * { return &material_; }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

private:
  auto LoadTextures(WCHAR **texture_filename_arr, unsigned int texture_count)
      -> bool;

  std::shared_ptr<DirectX12Device> device_;

  BumpMapMaterial material_;

  std::shared_ptr<ModelResource> model_resource_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_loader_ = nullptr;
};
