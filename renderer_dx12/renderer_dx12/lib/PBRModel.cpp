#include "stdafx.h"

#include "PBRModel.h"

#include <cmath>
#include <fstream>
#include <utility>

#include "DirectX12Device.h"

using namespace DirectX;
using namespace ResourceLoader;

PBRModel::PBRModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

PBRModel::~PBRModel() { ReleaseModel(); }

bool PBRModel::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) {
  if (!LoadModel(model_filename)) {
    return false;
  }

  CalculateModelVectors();

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

PBRMaterial *PBRModel::GetMaterial() { return &material_; }

DescriptorHeapPtr PBRModel::GetShaderRescourceView() const {
  return texture_container_->GetTexturesDescriptorHeap();
}

bool PBRModel::LoadModel(WCHAR *filename) {
  std::ifstream fin;
  fin.open(filename);
  if (fin.fail()) {
    return false;
  }

  char input = ' ';
  fin.get(input);
  while (input != ':') {
    fin.get(input);
  }

  fin >> vertex_count_;
  index_count_ = vertex_count_;

  model_data_ = new ModelType[vertex_count_];
  if (!model_data_) {
    return false;
  }

  fin.get(input);
  while (input != ':') {
    fin.get(input);
  }
  fin.get(input);
  fin.get(input);

  for (UINT i = 0; i < vertex_count_; ++i) {
    fin >> model_data_[i].x >> model_data_[i].y >> model_data_[i].z;
    fin >> model_data_[i].tu >> model_data_[i].tv;
    fin >> model_data_[i].nx >> model_data_[i].ny >> model_data_[i].nz;
    model_data_[i].tx = 0.0f;
    model_data_[i].ty = 0.0f;
    model_data_[i].tz = 0.0f;
    model_data_[i].bx = 0.0f;
    model_data_[i].by = 0.0f;
    model_data_[i].bz = 0.0f;
  }

  fin.close();
  return true;
}

void PBRModel::ReleaseModel() {
  if (model_data_) {
    delete[] model_data_;
    model_data_ = nullptr;
  }
}

void PBRModel::CalculateModelVectors() {
  if (!model_data_) {
    return;
  }

  UINT face_count = vertex_count_ / 3;
  UINT index = 0;

  for (UINT i = 0; i < face_count; ++i) {
    TempVertexType vertex1 = {};
    vertex1.x = model_data_[index].x;
    vertex1.y = model_data_[index].y;
    vertex1.z = model_data_[index].z;
    vertex1.tu = model_data_[index].tu;
    vertex1.tv = model_data_[index].tv;
    ++index;

    TempVertexType vertex2 = {};
    vertex2.x = model_data_[index].x;
    vertex2.y = model_data_[index].y;
    vertex2.z = model_data_[index].z;
    vertex2.tu = model_data_[index].tu;
    vertex2.tv = model_data_[index].tv;
    ++index;

    TempVertexType vertex3 = {};
    vertex3.x = model_data_[index].x;
    vertex3.y = model_data_[index].y;
    vertex3.z = model_data_[index].z;
    vertex3.tu = model_data_[index].tu;
    vertex3.tv = model_data_[index].tv;
    ++index;

    VectorType tangent = {};
    VectorType binormal = {};
    CalculateTangentBinormal(vertex1, vertex2, vertex3, tangent, binormal);

    model_data_[index - 1].tx = tangent.x;
    model_data_[index - 1].ty = tangent.y;
    model_data_[index - 1].tz = tangent.z;
    model_data_[index - 1].bx = binormal.x;
    model_data_[index - 1].by = binormal.y;
    model_data_[index - 1].bz = binormal.z;

    model_data_[index - 2].tx = tangent.x;
    model_data_[index - 2].ty = tangent.y;
    model_data_[index - 2].tz = tangent.z;
    model_data_[index - 2].bx = binormal.x;
    model_data_[index - 2].by = binormal.y;
    model_data_[index - 2].bz = binormal.z;

    model_data_[index - 3].tx = tangent.x;
    model_data_[index - 3].ty = tangent.y;
    model_data_[index - 3].tz = tangent.z;
    model_data_[index - 3].bx = binormal.x;
    model_data_[index - 3].by = binormal.y;
    model_data_[index - 3].bz = binormal.z;
  }
}

void PBRModel::CalculateTangentBinormal(const TempVertexType &vertex1,
                                        const TempVertexType &vertex2,
                                        const TempVertexType &vertex3,
                                        VectorType &tangent,
                                        VectorType &binormal) {
  float vector1[3] = {};
  float vector2[3] = {};
  float tu_vector[2] = {};
  float tv_vector[2] = {};

  vector1[0] = vertex2.x - vertex1.x;
  vector1[1] = vertex2.y - vertex1.y;
  vector1[2] = vertex2.z - vertex1.z;

  vector2[0] = vertex3.x - vertex1.x;
  vector2[1] = vertex3.y - vertex1.y;
  vector2[2] = vertex3.z - vertex1.z;

  tu_vector[0] = vertex2.tu - vertex1.tu;
  tv_vector[0] = vertex2.tv - vertex1.tv;

  tu_vector[1] = vertex3.tu - vertex1.tu;
  tv_vector[1] = vertex3.tv - vertex1.tv;

  float determinant = tu_vector[0] * tv_vector[1] - tu_vector[1] * tv_vector[0];
  if (fabsf(determinant) < 1e-6f) {
    tangent = {1.0f, 0.0f, 0.0f};
    binormal = {0.0f, 1.0f, 0.0f};
    return;
  }

  float denominator = 1.0f / determinant;

  tangent.x =
      (tv_vector[1] * vector1[0] - tv_vector[0] * vector2[0]) * denominator;
  tangent.y =
      (tv_vector[1] * vector1[1] - tv_vector[0] * vector2[1]) * denominator;
  tangent.z =
      (tv_vector[1] * vector1[2] - tv_vector[0] * vector2[2]) * denominator;

  binormal.x =
      (tu_vector[0] * vector2[0] - tu_vector[1] * vector1[0]) * denominator;
  binormal.y =
      (tu_vector[0] * vector2[1] - tu_vector[1] * vector1[1]) * denominator;
  binormal.z =
      (tu_vector[0] * vector2[2] - tu_vector[1] * vector1[2]) * denominator;

  float tangent_length =
      sqrtf((tangent.x * tangent.x) + (tangent.y * tangent.y) +
            (tangent.z * tangent.z));
  float binormal_length =
      sqrtf((binormal.x * binormal.x) + (binormal.y * binormal.y) +
            (binormal.z * binormal.z));

  if (tangent_length > 0.0f) {
    tangent.x /= tangent_length;
    tangent.y /= tangent_length;
    tangent.z /= tangent_length;
  }

  if (binormal_length > 0.0f) {
    binormal.x /= binormal_length;
    binormal.y /= binormal_length;
    binormal.z /= binormal_length;
  }
}

bool PBRModel::InitializeBuffers() {
  auto device = device_->GetD3d12Device();

  auto vertices = std::make_unique<VertexType[]>(vertex_count_);
  if (!vertices) {
    return false;
  }

  auto indices = std::make_unique<uint32_t[]>(index_count_);
  if (!indices) {
    return false;
  }

  for (UINT i = 0; i < vertex_count_; ++i) {
    vertices[i].position =
        XMFLOAT3(model_data_[i].x, model_data_[i].y, model_data_[i].z);
    vertices[i].texcoord = XMFLOAT2(model_data_[i].tu, model_data_[i].tv);
    vertices[i].normal =
        XMFLOAT3(model_data_[i].nx, model_data_[i].ny, model_data_[i].nz);
    vertices[i].tangent =
        XMFLOAT3(model_data_[i].tx, model_data_[i].ty, model_data_[i].tz);
    vertices[i].binormal =
        XMFLOAT3(model_data_[i].bx, model_data_[i].by, model_data_[i].bz);
    indices[i] = i;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) * vertex_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&vertex_buffer_)))) {
    return false;
  }

  UINT8 *vertex_data_begin = nullptr;
  if (FAILED(vertex_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&vertex_data_begin)))) {
    return false;
  }
  memcpy(vertex_data_begin, vertices.get(), sizeof(VertexType) * vertex_count_);
  vertex_buffer_->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint32_t) * index_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&index_buffer_)))) {
    return false;
  }

  UINT8 *index_data_begin = nullptr;
  if (FAILED(index_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&index_data_begin)))) {
    return false;
  }
  memcpy(index_data_begin, indices.get(), sizeof(uint32_t) * index_count_);
  index_buffer_->Unmap(0, nullptr);

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.SizeInBytes = sizeof(uint32_t) * index_count_;
  index_buffer_view_.Format = DXGI_FORMAT_R32_UINT;

  ReleaseModel();
  return true;
}

bool PBRModel::LoadTexture(WCHAR **texture_filename_arr) {
  texture_container_ = std::make_shared<TextureLoader>(device_);
  return texture_container_->LoadTexturesByNameArray(3, texture_filename_arr);
}
