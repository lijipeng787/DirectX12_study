#pragma once

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "ModelMaterial.h"
#include "TextureLoader.h"

class DirectX12Device;

class Model {
public:
  explicit Model(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

  Model(const Model &rhs) = delete;

  auto operator=(const Model &rhs) -> Model & = delete;

  ~Model() = default;

private:
  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
    DirectX::XMFLOAT3 normal_;
  };

  struct ModelType {
    float x_, y_, z_;
    float tu_, tv_;
    float nx_, ny_, nz_;
  };

public:
  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) -> bool;

  auto GetIndexCount() const -> UINT { return index_count_; }

  auto GetMaterial() -> ModelMaterial * { return &material_; }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;
  
  auto GetVertexBufferView() const -> const D3D12_VERTEX_BUFFER_VIEW & {
    return vertex_buffer_view_;
  }

  auto GetIndexBufferView() const -> const D3D12_INDEX_BUFFER_VIEW & {
    return index_buffer_view_;
  }

private:
  auto LoadModel(WCHAR *filename) -> bool;

  auto InitializeBuffers() -> bool;

  auto LoadTexture(WCHAR **texture_filename_arr) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ModelMaterial material_;

  ResourceSharedPtr vertex_buffer_ = nullptr;
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

  ResourceSharedPtr index_buffer_ = nullptr;
  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT vertex_count_ = 0;

  UINT index_count_ = 0;

  std::vector<ModelType> temp_model_;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;
};
