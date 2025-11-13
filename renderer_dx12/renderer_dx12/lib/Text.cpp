#include "stdafx.h"

#include "Text.h"

#include <utility>

#include "DirectX12Device.h"
#include "Font.h"

using namespace std;
using namespace DirectX;

Text::Text(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

Text::~Text() { ReleaseSentences(); }

bool Text::SetFps(int fps) {

  WCHAR tempString[16] = {};
  WCHAR fpsString[16] = {};

  if (fps > 9999) {
    fps = 9999;
  }

  _itow_s(fps, tempString, 10);
  lstrcpyW(fpsString, L"Fps: ");
  lstrcatW(fpsString, tempString);

  float red = 0.0f, green = 0.0f, blue = 0.0f;
  if (fps >= 60) {
    red = 0.0f;
    green = 1.0f;
    blue = 0.0f;
  }

  if (fps < 60) {
    red = 1.0f;
    green = 1.0f;
    blue = 0.0f;
  }

  if (fps < 30) {
    red = 1.0f;
    green = 0.0f;
    blue = 0.0f;
  }
  if (!UpdateSentenceVertexBuffer(sentence_vector_.at(1), fpsString, 20,
                                       20, red, green, blue)) {
    return false;
  }

  return true;
}

bool Text::SetCpu(int cpu_percentage_value) {

  WCHAR tempString[16] = {};
  WCHAR cpuString[16] = {};

  assert(cpu_percentage_value >= 0);
  _itow_s(cpu_percentage_value, tempString, 10);
  lstrcpyW(cpuString, L"Cpu: ");
  lstrcatW(cpuString, tempString);
  lstrcatW(cpuString, L"%");

  if (!UpdateSentenceVertexBuffer(sentence_vector_.at(0), cpuString, 20,
                                       40, 0.0f, 1.0f, 0.0f)) {
    return false;
  }

  return true;
}

bool Text::Initialize(int screen_width, int screen_height,
                      const DirectX::XMMATRIX &base_view_matrix) {

  ReleaseSentences();

  screen_width_ = screen_width;
  screen_height_ = screen_height;

  base_view_matrix_ = base_view_matrix;

  SentenceType *sentence1 = nullptr;
  if (!InitializeSentence(&sentence1, 16)) {
    delete sentence1;
    return false;
  }

  if (!UpdateSentenceVertexBuffer(sentence1, L"Hello", 100, 100, 1.0f,
                                       1.0f, 1.0f)) {
    delete sentence1;
    return false;
  }
  sentence_vector_.push_back(sentence1);

  SentenceType *sentence2 = nullptr;
  if (!InitializeSentence(&sentence2, 16)) {
    delete sentence2;
    return false;
  }

  if (!UpdateSentenceVertexBuffer(sentence2, L"World", 100, 200, 1.0f,
                                       1.0f, 0.0f)) {
    delete sentence2;
    return false;
  }
  sentence_vector_.push_back(sentence2);

  if (!material_.Initialize()) {
    return false;
  }

  return true;
}

bool Text::LoadFont(WCHAR *font_data, WCHAR **font_texture) {

  font_ = std::make_shared<BitmapFont>();
  if (!font_) {
    return false;
  }

  if (!font_->Initialize(font_data)) {
    return false;
  }
  if (!LoadTexture(font_texture)) {
    return false;
  }

  return true;
}

TextMaterial *Text::GetMaterial() { return &material_; }

bool Text::InitializeSentence(SentenceType **sentence, int max_length) {

  *sentence = new SentenceType;
  if (!*sentence) {
    return false;
  }

  (*sentence)->max_length_ = max_length;
  (*sentence)->vertex_count_ = 6 * max_length;
  (*sentence)->index_count_ = (*sentence)->vertex_count_;

  auto indices = new uint16_t[(*sentence)->index_count_];
  if (!indices) {
    return false;
  }

  for (auto i = 0; i < (*sentence)->index_count_; ++i) {
    indices[i] = i;
  }

  if (FAILED(device_->GetD3d12Device()->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) *
                                         (*sentence)->vertex_count_),
          D3D12_RESOURCE_STATE_COMMON, nullptr,
          IID_PPV_ARGS(&(*sentence)->vertex_buffer_)))) {
    return false;
  }

  (*sentence)->vertex_buffer_view_.BufferLocation =
      (*sentence)->vertex_buffer_->GetGPUVirtualAddress();
  (*sentence)->vertex_buffer_view_.SizeInBytes =
      sizeof(VertexType) * (*sentence)->vertex_count_;
  (*sentence)->vertex_buffer_view_.StrideInBytes = sizeof(VertexType);
  (*sentence)->vertex_buffer_state_ = D3D12_RESOURCE_STATE_COMMON;

  auto device = device_->GetD3d12Device();

  ResourceSharedPtr upload_index_buffer = nullptr;
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) *
                                         (*sentence)->index_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&upload_index_buffer)))) {
    return false;
  }

  if (FAILED(device_->GetD3d12Device()->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) *
                                         (*sentence)->index_count_),
          D3D12_RESOURCE_STATE_COMMON, nullptr,
          IID_PPV_ARGS(&(*sentence)->index_buffer_)))) {
    return false;
  }

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
  if (FAILED(device->CreateCommandQueue(&queue_desc,
                                        IID_PPV_ARGS(&command_queue)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
  if (FAILED(device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       command_allocator.Get(), nullptr,
                                       IID_PPV_ARGS(&command_list)))) {
    return false;
  }
  if (FAILED(command_list->Close())) {
    return false;
  }

  D3D12_SUBRESOURCE_DATA init_data = {};
  init_data.pData = indices;
  init_data.RowPitch = sizeof(uint16_t);
  init_data.SlicePitch = sizeof(uint16_t) * (*sentence)->index_count_;

  if (FAILED(command_allocator->Reset())) {
    return false;
  }

  if (FAILED(command_list->Reset(command_allocator.Get(), nullptr))) {
    return false;
  }

  if ((*sentence)->index_buffer_state_ != D3D12_RESOURCE_STATE_COPY_DEST) {
    auto to_copy = CD3DX12_RESOURCE_BARRIER::Transition(
        (*sentence)->index_buffer_.Get(), (*sentence)->index_buffer_state_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    command_list->ResourceBarrier(1, &to_copy);
    (*sentence)->index_buffer_state_ = D3D12_RESOURCE_STATE_COPY_DEST;
  }

  UpdateSubresources(command_list.Get(), (*sentence)->index_buffer_.Get(),
                     upload_index_buffer.Get(), 0, 0, 1, &init_data);

  auto to_index = CD3DX12_RESOURCE_BARRIER::Transition(
      (*sentence)->index_buffer_.Get(), (*sentence)->index_buffer_state_,
      D3D12_RESOURCE_STATE_INDEX_BUFFER);
  command_list->ResourceBarrier(1, &to_index);
  (*sentence)->index_buffer_state_ = D3D12_RESOURCE_STATE_INDEX_BUFFER;

  if (FAILED(command_list->Close())) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(&fence)))) {
    return false;
  }

  auto fence_event = CreateEvent(nullptr, false, false, nullptr);
  if (nullptr == fence_event) {
    return false;
  }

  ID3D12CommandList *ppCommandLists[] = {command_list.Get()};
  command_queue->ExecuteCommandLists(1, ppCommandLists);

  if (FAILED(command_queue->Signal(fence.Get(), 1))) {
    return false;
  }

  if (fence->GetCompletedValue() < 1) {
    if (FAILED(fence->SetEventOnCompletion(1, fence_event))) {
      return false;
    }
    WaitForSingleObject(fence_event, INFINITE);
  }

  CloseHandle(fence_event);

  (*sentence)->index_buffer_view_.BufferLocation =
      (*sentence)->index_buffer_->GetGPUVirtualAddress();
  (*sentence)->index_buffer_view_.SizeInBytes =
      sizeof(uint16_t) * (*sentence)->index_count_;
  (*sentence)->index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;
  (*sentence)->index_buffer_state_ = D3D12_RESOURCE_STATE_INDEX_BUFFER;

  delete[] indices;
  indices = nullptr;

  return true;
}

DescriptorHeapPtr Text::GetShaderResourceView() const {
  return texture_container_->GetTexturesDescriptorHeap();
}

bool Text::LoadTexture(WCHAR **filename_arr) {

  texture_container_ = std::make_shared<ResourceLoader::TextureLoader>(device_);
  if (!texture_container_) {
    return false;
  }
  if (!texture_container_->LoadTexturesByNameArray(1, filename_arr)) {
    return false;
  }

  return true;
}

bool Text::UpdateSentenceVertexBuffer(SentenceType *sentence, WCHAR *text,
                                      int positionX, int positionY, float red,
                                      float green, float blue) {

  sentence->red_ = red;
  sentence->green_ = green;
  sentence->blue_ = blue;

  auto numLetters = (int)wcslen(text);
  if (numLetters > sentence->max_length_) {
    return false;
  }

  auto vertices = new VertexType[sentence->vertex_count_];
  if (!vertices) {
    return false;
  }

  memset(vertices, 0, (sizeof(VertexType) * sentence->vertex_count_));

  auto drawX = static_cast<float>(((screen_width_ / 2) * -1) + positionX);
  auto drawY = static_cast<float>((screen_height_ / 2) - positionY);

  font_->BuildVertexArray((void *)vertices, text, drawX, drawY);

  auto device = device_->GetD3d12Device();

  ResourceSharedPtr upload_vertex_buffer = nullptr;
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) *
                                         sentence->vertex_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&upload_vertex_buffer)))) {
    return false;
  }

  D3D12_SUBRESOURCE_DATA init_data = {};
  init_data.pData = vertices;
  init_data.RowPitch = sizeof(VertexType);
  init_data.SlicePitch = sizeof(VertexType) * sentence->vertex_count_;

  auto command_allocator = device_->GetDefaultGraphicsCommandAllocator().Get();
  auto command_list = device_->GetDefaultGraphicsCommandList().Get();

  if (FAILED(command_allocator->Reset())) {
    return false;
  }
  if (FAILED(command_list->Reset(command_allocator, nullptr))) {
    return false;
  }

  if (sentence->vertex_buffer_state_ != D3D12_RESOURCE_STATE_COPY_DEST) {
    auto to_copy = CD3DX12_RESOURCE_BARRIER::Transition(
        sentence->vertex_buffer_.Get(), sentence->vertex_buffer_state_,
        D3D12_RESOURCE_STATE_COPY_DEST);
    command_list->ResourceBarrier(1, &to_copy);
    sentence->vertex_buffer_state_ = D3D12_RESOURCE_STATE_COPY_DEST;
  }

  UpdateSubresources(command_list, sentence->vertex_buffer_.Get(),
                     upload_vertex_buffer.Get(), 0, 0, 1, &init_data);

  auto to_vertex = CD3DX12_RESOURCE_BARRIER::Transition(
      sentence->vertex_buffer_.Get(), sentence->vertex_buffer_state_,
      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
  command_list->ResourceBarrier(1, &to_vertex);
  sentence->vertex_buffer_state_ =
      D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

  if (FAILED(command_list->Close())) {
    return false;
  }

  auto command_queue = device_->GetDefaultGraphicsCommandQueeue().Get();
  ID3D12CommandList *ppCommandLists[] = {command_list};
  command_queue->ExecuteCommandLists(1, ppCommandLists);

  auto fence_event = CreateEvent(nullptr, false, false, nullptr);
  if (nullptr == fence_event) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  if (FAILED(device_->GetD3d12Device()->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                                    IID_PPV_ARGS(&fence)))) {
    return false;
  }

  if (FAILED(command_queue->Signal(fence.Get(), 1))) {
    return false;
  }

  if (fence->GetCompletedValue() < 1) {
    if (FAILED(fence->SetEventOnCompletion(1, fence_event))) {
      return false;
    }
    WaitForSingleObject(fence_event, INFINITE);
  }

  CloseHandle(fence_event);

  delete[] vertices;
  vertices = nullptr;

  return true;
}

void Text::ReleaseSentences() {
  for (auto sentence : sentence_vector_) {
    delete sentence;
  }
  sentence_vector_.clear();
}