#include "stdafx.h"

#include "Model.h"

#include <utility>
#include <vector>
#include <fstream>

#include "DirectX12Device.h"
#include "ModelMaterial.h"

using namespace DirectX;
using namespace ResourceLoader;

namespace {

constexpr UINT kTextureCount = 3;

bool SkipUntil(std::istream &stream, char delimiter) {
  char ch = 0;
  while (stream.get(ch)) {
    if (ch == delimiter) {
      return true;
    }
  }
  return false;
}

} // namespace

bool Model::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) {

  if (!LoadModel(model_filename)) {
    return false;
  }
  if (!InitializeBuffers()) {
    return false;
  }
  if (!LoadTexture(texture_filename_arr)) {
    return false;
  }
  if (!material_.Initialize()) {
    return false;
  }

  return true;
}

DescriptorHeapPtr Model::GetShaderResourceView() const {
  if (!texture_container_) {
    return {};
  }
  return texture_container_->GetTexturesDescriptorHeap();
}

bool Model::LoadModel(WCHAR *filename) {

  std::ifstream fin(filename);
  if (!fin.is_open()) {
    return false;
  }

  if (!SkipUntil(fin, ':')) {
    return false;
  }

  fin >> vertex_count_;
  if (!fin) {
    return false;
  }
  index_count_ = vertex_count_;

  temp_model_.clear();
  temp_model_.resize(vertex_count_);

  if (!SkipUntil(fin, ':')) {
    return false;
  }
  // Skip the whitespace/newline characters after the colon to reach data lines.
  fin.get();
  fin.get();

  for (UINT i = 0; i < vertex_count_; ++i) {
    if (!(fin >> temp_model_[i].x_ >> temp_model_[i].y_ >> temp_model_[i].z_ >>
          temp_model_[i].tu_ >> temp_model_[i].tv_ >> temp_model_[i].nx_ >>
          temp_model_[i].ny_ >> temp_model_[i].nz_)) {
      return false;
    }
  }

  return true;
}

bool Model::InitializeBuffers() {

  if (vertex_count_ == 0 || index_count_ == 0 ||
      temp_model_.size() != vertex_count_) {
    return false;
  }

  std::vector<VertexType> vertices(vertex_count_);
  std::vector<uint16_t> indices(index_count_);

  for (UINT i = 0; i < vertex_count_; ++i) {
    vertices[i].position_ =
        DirectX::XMFLOAT3(temp_model_[i].x_, temp_model_[i].y_, temp_model_[i].z_);
    vertices[i].texture_position_ =
        DirectX::XMFLOAT2(temp_model_[i].tu_, temp_model_[i].tv_);
    vertices[i].normal_ = DirectX::XMFLOAT3(
        temp_model_[i].nx_, temp_model_[i].ny_, temp_model_[i].nz_);
    indices[i] = static_cast<uint16_t>(i);
  }

  const size_t vertex_buffer_size = sizeof(VertexType) * vertex_count_;
  if (!device_->CreateBuffer(vertex_buffer_size, vertices.data(),
                             vertex_buffer_,
                             D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)) {
    return false;
  }

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

  const size_t index_buffer_size = sizeof(uint16_t) * index_count_;
  if (!device_->CreateBuffer(index_buffer_size, indices.data(), index_buffer_,
                             D3D12_RESOURCE_STATE_INDEX_BUFFER)) {
    return false;
  }

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

  temp_model_.clear();
  temp_model_.shrink_to_fit();

  return true;
}

bool Model::LoadTexture(WCHAR **texture_filename_arr) {
  texture_container_ = std::make_shared<TextureLoader>(device_);
  return texture_container_->LoadTexturesByNameArray(kTextureCount, texture_filename_arr);
}