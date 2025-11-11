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

  temp_model_.clear();
  temp_model_.shrink_to_fit();

  return true;
}

bool Model::LoadTexture(WCHAR **texture_filename_arr) {
  texture_container_ = std::make_shared<TextureLoader>(device_);
  return texture_container_->LoadTexturesByNameArray(kTextureCount, texture_filename_arr);
}