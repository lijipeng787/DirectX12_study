#include "stdafx.h"

#include "BumpMapModel.h"

#include <cmath>
#include <fstream>

#include "DirectX12Device.h"

using namespace DirectX;
using namespace ResourceLoader;

BumpMapModel::BumpMapModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

BumpMapModel::~BumpMapModel() { ReleaseModel(); }

auto BumpMapModel::Initialize(WCHAR *model_filename,
                              WCHAR **texture_filename_arr,
                              unsigned int texture_count) -> bool {
  if (!device_ || texture_count == 0) {
    return false;
  }

  if (!LoadModel(model_filename)) {
    return false;
  }

  CalculateModelVectors();

  if (!InitializeBuffers()) {
    return false;
  }

  if (!LoadTextures(texture_filename_arr, texture_count)) {
    return false;
  }

  if (!material_.Initialize()) {
    return false;
  }

  return true;
}

auto BumpMapModel::GetShaderResourceView() const -> DescriptorHeapPtr {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTexturesDescriptorHeap();
}

auto BumpMapModel::LoadModel(WCHAR *filename) -> bool {
  std::ifstream fin;
  fin.open(filename);
  if (fin.fail()) {
    return false;
  }

  char input = ' ';
  fin.get(input);
  while (input != ':' && !fin.eof()) {
    fin.get(input);
  }

  fin >> vertex_count_;
  index_count_ = vertex_count_;

  model_data_ = new ModelType[vertex_count_];
  if (!model_data_) {
    return false;
  }

  fin.get(input);
  while (input != ':' && !fin.eof()) {
    fin.get(input);
  }
  fin.get(input);
  fin.get(input);

  for (UINT i = 0; i < vertex_count_; ++i) {
    fin >> model_data_[i].x >> model_data_[i].y >> model_data_[i].z;
    fin >> model_data_[i].tu >> model_data_[i].tv;
    fin >> model_data_[i].nx >> model_data_[i].ny >> model_data_[i].nz;

    model_data_[i].tx = model_data_[i].ty = model_data_[i].tz = 0.0f;
    model_data_[i].bx = model_data_[i].by = model_data_[i].bz = 0.0f;
  }

  fin.close();
  return true;
}

void BumpMapModel::ReleaseModel() {
  if (model_data_) {
    delete[] model_data_;
    model_data_ = nullptr;
  }
}

auto BumpMapModel::InitializeBuffers() -> bool {
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
    vertices[i].tangent =
        XMFLOAT3(model_data_[i].tx, model_data_[i].ty, model_data_[i].tz);
    vertices[i].binormal =
        XMFLOAT3(model_data_[i].bx, model_data_[i].by, model_data_[i].bz);

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
  memcpy(vertex_data_begin, vertices.get(), sizeof(VertexType) * vertex_count_);
  vertex_buffer_->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;

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
  memcpy(index_data_begin, indices.get(), sizeof(uint16_t) * index_count_);
  index_buffer_->Unmap(0, nullptr);

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
  index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;

  // Model data no longer needed after vertex buffer creation.
  ReleaseModel();

  return true;
}

auto BumpMapModel::LoadTextures(WCHAR **texture_filename_arr,
                                unsigned int texture_count) -> bool {
  if (!device_) {
    return false;
  }

  texture_loader_ = std::make_shared<TextureLoader>(device_);
  if (!texture_loader_) {
    return false;
  }

  return texture_loader_->LoadTexturesByNameArray(texture_count,
                                                  texture_filename_arr);
}

void BumpMapModel::CalculateModelVectors() {
  if (!model_data_ || vertex_count_ < 3) {
    return;
  }

  const int face_count = static_cast<int>(vertex_count_) / 3;
  int index = 0;

  for (int i = 0; i < face_count; ++i) {
    TempVertexType vertex1 = {};
    TempVertexType vertex2 = {};
    TempVertexType vertex3 = {};

    vertex1.x = model_data_[index].x;
    vertex1.y = model_data_[index].y;
    vertex1.z = model_data_[index].z;
    vertex1.tu = model_data_[index].tu;
    vertex1.tv = model_data_[index].tv;
    vertex1.nx = model_data_[index].nx;
    vertex1.ny = model_data_[index].ny;
    vertex1.nz = model_data_[index].nz;
    ++index;

    vertex2.x = model_data_[index].x;
    vertex2.y = model_data_[index].y;
    vertex2.z = model_data_[index].z;
    vertex2.tu = model_data_[index].tu;
    vertex2.tv = model_data_[index].tv;
    vertex2.nx = model_data_[index].nx;
    vertex2.ny = model_data_[index].ny;
    vertex2.nz = model_data_[index].nz;
    ++index;

    vertex3.x = model_data_[index].x;
    vertex3.y = model_data_[index].y;
    vertex3.z = model_data_[index].z;
    vertex3.tu = model_data_[index].tu;
    vertex3.tv = model_data_[index].tv;
    vertex3.nx = model_data_[index].nx;
    vertex3.ny = model_data_[index].ny;
    vertex3.nz = model_data_[index].nz;
    ++index;

    VectorType tangent = {};
    VectorType binormal = {};
    CalculateTangentBinormal(vertex1, vertex2, vertex3, tangent, binormal);

    VectorType normal = {};
    CalculateNormal(tangent, binormal, normal);

    model_data_[index - 1].nx = normal.x;
    model_data_[index - 1].ny = normal.y;
    model_data_[index - 1].nz = normal.z;
    model_data_[index - 1].tx = tangent.x;
    model_data_[index - 1].ty = tangent.y;
    model_data_[index - 1].tz = tangent.z;
    model_data_[index - 1].bx = binormal.x;
    model_data_[index - 1].by = binormal.y;
    model_data_[index - 1].bz = binormal.z;

    model_data_[index - 2].nx = normal.x;
    model_data_[index - 2].ny = normal.y;
    model_data_[index - 2].nz = normal.z;
    model_data_[index - 2].tx = tangent.x;
    model_data_[index - 2].ty = tangent.y;
    model_data_[index - 2].tz = tangent.z;
    model_data_[index - 2].bx = binormal.x;
    model_data_[index - 2].by = binormal.y;
    model_data_[index - 2].bz = binormal.z;

    model_data_[index - 3].nx = normal.x;
    model_data_[index - 3].ny = normal.y;
    model_data_[index - 3].nz = normal.z;
    model_data_[index - 3].tx = tangent.x;
    model_data_[index - 3].ty = tangent.y;
    model_data_[index - 3].tz = tangent.z;
    model_data_[index - 3].bx = binormal.x;
    model_data_[index - 3].by = binormal.y;
    model_data_[index - 3].bz = binormal.z;
  }
}

void BumpMapModel::CalculateTangentBinormal(const TempVertexType &vertex1,
                                            const TempVertexType &vertex2,
                                            const TempVertexType &vertex3,
                                            VectorType &tangent,
                                            VectorType &binormal) {
  const float vector1[3] = {vertex2.x - vertex1.x, vertex2.y - vertex1.y,
                            vertex2.z - vertex1.z};
  const float vector2[3] = {vertex3.x - vertex1.x, vertex3.y - vertex1.y,
                            vertex3.z - vertex1.z};

  const float tu_vector[2] = {vertex2.tu - vertex1.tu,
                              vertex3.tu - vertex1.tu};
  const float tv_vector[2] = {vertex2.tv - vertex1.tv,
                              vertex3.tv - vertex1.tv};

  float denominator = tu_vector[0] * tv_vector[1] - tu_vector[1] * tv_vector[0];
  if (std::fabs(denominator) < 1.0e-6f) {
    denominator = 1.0f;
  } else {
    denominator = 1.0f / denominator;
  }

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

  XMVECTOR tangent_vec =
      XMVector3Normalize(XMVectorSet(tangent.x, tangent.y, tangent.z, 0.0f));
  XMVECTOR binormal_vec =
      XMVector3Normalize(XMVectorSet(binormal.x, binormal.y, binormal.z, 0.0f));

  XMFLOAT3 tangent_float = {};
  XMFLOAT3 binormal_float = {};
  XMStoreFloat3(&tangent_float, tangent_vec);
  XMStoreFloat3(&binormal_float, binormal_vec);

  tangent.x = tangent_float.x;
  tangent.y = tangent_float.y;
  tangent.z = tangent_float.z;

  binormal.x = binormal_float.x;
  binormal.y = binormal_float.y;
  binormal.z = binormal_float.z;
}

void BumpMapModel::CalculateNormal(const VectorType &tangent,
                                   const VectorType &binormal,
                                   VectorType &normal) {
  XMVECTOR tangent_vec =
      XMVectorSet(tangent.x, tangent.y, tangent.z, 0.0f);
  XMVECTOR binormal_vec =
      XMVectorSet(binormal.x, binormal.y, binormal.z, 0.0f);

  XMVECTOR normal_vec = XMVector3Normalize(XMVector3Cross(tangent_vec, binormal_vec));

  XMFLOAT3 normal_float = {};
  XMStoreFloat3(&normal_float, normal_vec);

  normal.x = normal_float.x;
  normal.y = normal_float.y;
  normal.z = normal_float.z;
}

