#include "stdafx.h"

#include "ReflectionModel.h"

#include <fstream>
#include <utility>

#include "DirectX12Device.h"

using namespace DirectX;
using namespace ResourceLoader;

ReflectionModel::ReflectionModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

ReflectionModel::~ReflectionModel() { ReleaseModel(); }

bool ReflectionModel::Initialize(WCHAR *model_filename,
                                 WCHAR **texture_filename_arr,
                                 unsigned int texture_count) {
  if (!LoadModel(model_filename)) {
    return false;
  }

  if (!InitializeBuffers()) {
    return false;
  }

  if (!LoadTextures(texture_filename_arr, texture_count)) {
    return false;
  }

  ReleaseModel();
  return true;
}

DescriptorHeapPtr ReflectionModel::GetShaderResourceView() const {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTexturesDescriptorHeap();
}

ResourceSharedPtr ReflectionModel::GetTextureResource(size_t index) const {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTextureResource(index);
}

bool ReflectionModel::LoadModel(WCHAR *filename) {
  std::ifstream fin;
  fin.open(filename);
  if (fin.fail()) {
    return false;
  }

  char input = ' ';
  while (input != ':' && fin.good()) {
    fin.get(input);
  }

  fin >> vertex_count_;
  index_count_ = vertex_count_;

  model_data_ = new ModelType[vertex_count_];
  if (!model_data_) {
    return false;
  }

  fin.get(input);
  while (input != ':' && fin.good()) {
    fin.get(input);
  }
  fin.get(input);
  fin.get(input);

  for (UINT i = 0; i < vertex_count_; ++i) {
    fin >> model_data_[i].x >> model_data_[i].y >> model_data_[i].z;
    fin >> model_data_[i].tu >> model_data_[i].tv;
    fin >> model_data_[i].nx >> model_data_[i].ny >> model_data_[i].nz;
  }

  fin.close();
  return true;
}

bool ReflectionModel::InitializeBuffers() {
  if (!device_ || !model_data_) {
    return false;
  }

  auto vertices = std::make_unique<VertexType[]>(vertex_count_);
  auto indices = std::make_unique<uint16_t[]>(index_count_);
  if (!vertices || !indices) {
    return false;
  }

  for (UINT i = 0; i < vertex_count_; ++i) {
    vertices[i].position =
        XMFLOAT3(model_data_[i].x, model_data_[i].y, model_data_[i].z);
    vertices[i].texcoord =
        XMFLOAT2(model_data_[i].tu, model_data_[i].tv);
    vertices[i].normal =
        XMFLOAT3(model_data_[i].nx, model_data_[i].ny, model_data_[i].nz);
    indices[i] = static_cast<uint16_t>(i);
  }

  auto d3d_device = device_->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  if (FAILED(d3d_device->CreateCommittedResource(
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

  memcpy(vertex_data_begin, vertices.get(),
         sizeof(VertexType) * vertex_count_);
  vertex_buffer_->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

  if (FAILED(d3d_device->CreateCommittedResource(
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

  memcpy(index_data_begin, indices.get(), sizeof(uint16_t) * index_count_);
  index_buffer_->Unmap(0, nullptr);

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

  return true;
}

bool ReflectionModel::LoadTextures(WCHAR **texture_filename_arr,
                                   unsigned int texture_count) {
  texture_loader_ = std::make_shared<TextureLoader>(device_);
  if (!texture_loader_) {
    return false;
  }

  return texture_loader_->LoadTexturesByNameArray(texture_count,
                                                  texture_filename_arr);
}

void ReflectionModel::ReleaseModel() {
  if (model_data_) {
    delete[] model_data_;
    model_data_ = nullptr;
  }
}

