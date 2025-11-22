#include "stdafx.h"

#include "ScreenQuad.h"

#include <utility>

#include "DirectX12Device.h"

using namespace std;
using namespace DirectX;

ScreenQuad::ScreenQuad(std::shared_ptr<DirectX12Device> device,
                       std::shared_ptr<ScreenQuadMaterial> material)
    : device_(std::move(device)), material_(std::move(material)) {}

bool ScreenQuad::Initialize(UINT screen_width, UINT screen_height,
                            UINT bitmap_width, UINT bitmap_height) {

  screen_width_ = screen_width;
  screen_height_ = screen_height;

  quad_width_ = bitmap_width;
  quad_height_ = bitmap_height;

  if (!material_) {
    Logger::Error(L"[ScreenQuad] Initialize failed: material is null.");
    return false;
  }

  if (!InitializeBuffers()) {
    return false;
  }
  if (!material_->IsInitialized() && !material_->Initialize()) {
    return false;
  }

  return true;
}

void ScreenQuad::SetIndexBufferView(const IndexBufferView &view) {
  index_buffer_view_ = view;
  index_count_ =
      static_cast<UINT>(index_buffer_view_.SizeInBytes / sizeof(uint16_t));
}

std::shared_ptr<ScreenQuadMaterial> ScreenQuad::GetMaterialShared() const {
  return material_;
}

void ScreenQuad::SetMaterial(std::shared_ptr<ScreenQuadMaterial> material) {
  if (!material) {
    return;
  }
  material_ = std::move(material);
}

bool ScreenQuad::UpdatePosition(int pos_x, int pos_y) {

  if (last_pos_x_ == pos_x && last_pos_y_ == pos_y) {
    return true;
  }

  last_pos_x_ = pos_x;
  last_pos_y_ = pos_y;

  auto left =
      (static_cast<float>(screen_width_) / 2) * -1 + static_cast<float>(pos_x);

  auto right = left + static_cast<float>(quad_width_);

  auto top = static_cast<float>(screen_height_) / 2 - static_cast<float>(pos_y);

  auto bottom = top - static_cast<float>(quad_height_);

  auto vertices = new VertexType[vertex_count_];
  if (!vertices) {
    return false;
  }

  // First triangle.
  // Top left.
  vertices[0].position_ = DirectX::XMFLOAT3(left, top, 0.0f);
  vertices[0].texture_position_ = DirectX::XMFLOAT2(0.0f, 0.0f);
  // Bottom right.
  vertices[1].position_ = DirectX::XMFLOAT3(right, bottom, 0.0f);
  vertices[1].texture_position_ = DirectX::XMFLOAT2(1.0f, 1.0f);
  // Bottom left.
  vertices[2].position_ = DirectX::XMFLOAT3(left, bottom, 0.0f);
  vertices[2].texture_position_ = DirectX::XMFLOAT2(0.0f, 1.0f);

  // Second triangle.
  // Top left.
  vertices[3].position_ = DirectX::XMFLOAT3(left, top, 0.0f);
  vertices[3].texture_position_ = DirectX::XMFLOAT2(0.0f, 0.0f);
  // Top right.
  vertices[4].position_ = DirectX::XMFLOAT3(right, top, 0.0f);
  vertices[4].texture_position_ = DirectX::XMFLOAT2(1.0f, 0.0f);
  // Bottom right.
  vertices[5].position_ = DirectX::XMFLOAT3(right, bottom, 0.0f);
  vertices[5].texture_position_ = DirectX::XMFLOAT2(1.0f, 1.0f);

  D3D12_RANGE range;
  range.Begin = 0;
  range.End = 0;
  UINT8 *data_begin = nullptr;
  if (FAILED(vertex_buffer_->Map(0, &range,
                                 reinterpret_cast<void **>(&data_begin)))) {
    return false;
  } else {
    memcpy(data_begin, vertices, sizeof(VertexType) * vertex_count_);
    vertex_buffer_->Unmap(0, nullptr);
  }

  delete[] vertices;
  vertices = nullptr;

  return true;
}

bool ScreenQuad::InitializeBuffers() {
  vertex_count_ = 6;
  index_count_ = 6;

  auto vertices = new VertexType[vertex_count_];
  if (!vertices) {
    return false;
  }

  auto indices = new uint16_t[index_count_];
  if (!indices) {
    return false;
  }

  ZeroMemory(vertices, sizeof(VertexType) * vertex_count_);

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

  memcpy(vertex_data_begin, vertices, sizeof(VertexType) * vertex_count_);
  vertex_buffer_->Unmap(0, nullptr);

  vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
  vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * vertex_count_;
  vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

  delete[] vertices;
  vertices = nullptr;

  for (UINT i = 0; i < index_count_; ++i) {
    indices[i] = i;
  }

  if (!device_->CreateBuffer(sizeof(uint16_t) * index_count_, indices,
                             index_buffer_, D3D12_RESOURCE_STATE_INDEX_BUFFER)) {
    return false;
  }

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

  delete[] indices;
  indices = nullptr;

  return true;
}