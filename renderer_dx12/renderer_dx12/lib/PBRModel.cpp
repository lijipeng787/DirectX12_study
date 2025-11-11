#include "stdafx.h"

#include "PBRModel.h"

#include <cmath>
#include <fstream>
#include <utility>
#include <vector>

#include "DirectX12Device.h"

using namespace DirectX;
using namespace ResourceLoader;

namespace {

bool CreateBufferOnGpu(const std::shared_ptr<DirectX12Device> &device,
                       size_t buffer_size, const void *source_data,
                       D3D12_RESOURCE_STATES final_state,
                       ResourceSharedPtr &default_buffer) {
  if (!device || buffer_size == 0 || source_data == nullptr) {
    return false;
  }

  auto d3d_device = device->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  ResourceSharedPtr upload_buffer = nullptr;

  if (FAILED(d3d_device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
          D3D12_RESOURCE_STATE_COMMON, nullptr,
          IID_PPV_ARGS(&default_buffer)))) {
    return false;
  }

  if (FAILED(d3d_device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(buffer_size),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&upload_buffer)))) {
    return false;
  }

  UINT8 *mapped_data = nullptr;
  if (FAILED(upload_buffer->Map(
          0, nullptr, reinterpret_cast<void **>(&mapped_data)))) {
    return false;
  }
  memcpy(mapped_data, source_data, buffer_size);
  upload_buffer->Unmap(0, nullptr);

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  if (FAILED(d3d_device->CreateCommandQueue(&queue_desc,
                                            IID_PPV_ARGS(&command_queue)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
  if (FAILED(d3d_device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT,
          IID_PPV_ARGS(&command_allocator)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
  if (FAILED(d3d_device->CreateCommandList(
          0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), nullptr,
          IID_PPV_ARGS(&command_list)))) {
    return false;
  }

  auto to_copy_dest = CD3DX12_RESOURCE_BARRIER::Transition(
      default_buffer.Get(), D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  command_list->ResourceBarrier(1, &to_copy_dest);

  D3D12_SUBRESOURCE_DATA subresource = {};
  subresource.pData = source_data;
  subresource.RowPitch = buffer_size;
  subresource.SlicePitch = buffer_size;

  UpdateSubresources(command_list.Get(), default_buffer.Get(),
                     upload_buffer.Get(), 0, 0, 1, &subresource);

  auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
      default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, final_state);
  command_list->ResourceBarrier(1, &barrier);

  if (FAILED(command_list->Close())) {
    return false;
  }

  ID3D12CommandList *lists[] = {command_list.Get()};
  command_queue->ExecuteCommandLists(1, lists);

  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  if (FAILED(d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                     IID_PPV_ARGS(&fence)))) {
    return false;
  }

  HANDLE event_handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!event_handle) {
    return false;
  }

  if (FAILED(command_queue->Signal(fence.Get(), 1))) {
    CloseHandle(event_handle);
    return false;
  }

  if (fence->GetCompletedValue() < 1) {
    if (FAILED(fence->SetEventOnCompletion(1, event_handle))) {
      CloseHandle(event_handle);
      return false;
    }
    WaitForSingleObject(event_handle, INFINITE);
  }

  CloseHandle(event_handle);
  return true;
}

} // namespace
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
  if (!device_ || !model_data_) {
    return false;
  }

  std::vector<VertexType> vertices(vertex_count_);
  std::vector<uint32_t> indices(index_count_);

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

  const size_t vertex_buffer_size = sizeof(VertexType) * vertex_count_;
  if (!CreateBufferOnGpu(device_, vertex_buffer_size, vertices.data(),
                         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                         vertex_buffer_)) {
    return false;
  }

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;

  const size_t index_buffer_size = sizeof(uint32_t) * index_count_;
  if (!CreateBufferOnGpu(device_, index_buffer_size, indices.data(),
                         D3D12_RESOURCE_STATE_INDEX_BUFFER, index_buffer_)) {
    return false;
  }

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
