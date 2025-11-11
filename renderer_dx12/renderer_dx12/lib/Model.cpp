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

DescriptorHeapPtr Model::GetShaderRescourceView() const {
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

  auto device = device_->GetD3d12Device();

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) * vertex_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&vertex_buffer_)))) {
    return false;
  }

  UINT8 *vertex_data_begin = nullptr;
  CD3DX12_RANGE read_range(0, 0);
  if (FAILED(vertex_buffer_->Map(
          0, &read_range, reinterpret_cast<void **>(&vertex_data_begin)))) {
    return false;
  }

  memcpy(vertex_data_begin, vertices.data(), sizeof(VertexType) * vertex_count_);
  vertex_buffer_->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * index_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&index_buffer_)))) {
    return false;
  }

  UINT8 *index_data_begin = nullptr;
  if (FAILED(index_buffer_->Map(
          0, &read_range, reinterpret_cast<void **>(&index_data_begin)))) {
    return false;
  }

  memcpy(index_data_begin, indices.data(), sizeof(uint16_t) * index_count_);
  index_buffer_->Unmap(0, nullptr);

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