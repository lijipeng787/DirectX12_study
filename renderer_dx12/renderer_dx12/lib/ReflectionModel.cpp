#include "stdafx.h"

#include "ReflectionModel.h"

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

  std::vector<VertexType> vertices(vertex_count_);
  std::vector<uint16_t> indices(index_count_);

  for (UINT i = 0; i < vertex_count_; ++i) {
    vertices[i].position =
        XMFLOAT3(model_data_[i].x, model_data_[i].y, model_data_[i].z);
    vertices[i].texcoord =
        XMFLOAT2(model_data_[i].tu, model_data_[i].tv);
    vertices[i].normal =
        XMFLOAT3(model_data_[i].nx, model_data_[i].ny, model_data_[i].nz);
    indices[i] = static_cast<uint16_t>(i);
  }

  const size_t vertex_buffer_size = sizeof(VertexType) * vertex_count_;
  if (!CreateBufferOnGpu(device_, vertex_buffer_size, vertices.data(),
                         D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                         vertex_buffer_)) {
    return false;
  }

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

  const size_t index_buffer_size = sizeof(uint16_t) * index_count_;
  if (!CreateBufferOnGpu(device_, index_buffer_size, indices.data(),
                         D3D12_RESOURCE_STATE_INDEX_BUFFER, index_buffer_)) {
    return false;
  }

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

