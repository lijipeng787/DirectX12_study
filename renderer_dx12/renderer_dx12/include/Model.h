#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "ModelMaterial.h"
#include "TextureLoader.h"
#include "ResourceManager.h"

class DirectX12Device;

class Model {
public:
  explicit Model(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

  Model(const Model &rhs) = delete;

  auto operator=(const Model &rhs) -> Model & = delete;

  ~Model() = default;

public:
  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) -> bool;

  auto GetIndexCount() const -> UINT { 
      return model_resource_ ? model_resource_->index_count : 0; 
  }

  auto GetMaterial() -> ModelMaterial * { return &material_; }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;
  
  auto GetVertexBufferView() const -> const D3D12_VERTEX_BUFFER_VIEW & {
    static D3D12_VERTEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->vertex_buffer_view : null_view;
  }

  auto GetIndexBufferView() const -> const D3D12_INDEX_BUFFER_VIEW & {
    static D3D12_INDEX_BUFFER_VIEW null_view = {};
    return model_resource_ ? model_resource_->index_buffer_view : null_view;
  }

private:
  auto LoadTexture(WCHAR **texture_filename_arr) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ModelMaterial material_;

  std::shared_ptr<ModelResource> model_resource_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;
};
