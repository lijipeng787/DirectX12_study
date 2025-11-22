#pragma once

#include <DirectXMath.h>
#include <memory>

#include "PBRMaterial.h"
#include "TextureLoader.h"
#include "ResourceManager.h"

class DirectX12Device;

class PBRModel {
public:
  explicit PBRModel(std::shared_ptr<DirectX12Device> device);

  PBRModel(const PBRModel &rhs) = delete;

  auto operator=(const PBRModel &rhs) -> PBRModel & = delete;

  ~PBRModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) -> bool;

  auto GetIndexCount() const -> UINT { return model_resource_ ? model_resource_->index_count : 0; }

  auto GetMaterial() -> PBRMaterial *;

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const {
    static D3D12_VERTEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->vertex_buffer_view : null_view;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const {
    static D3D12_INDEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->index_buffer_view : null_view;
  }

private:
  auto LoadTexture(WCHAR **texture_filename_arr) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  PBRMaterial material_;

  std::shared_ptr<ModelResource> model_resource_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;
};
