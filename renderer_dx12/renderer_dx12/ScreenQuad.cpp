#include "stdafx.h"

#include "ScreenQuad.h"

#include <utility>

#include "DirectX12Device.h"

using namespace std;
using namespace DirectX;

ScreenQuad::ScreenQuad(std::shared_ptr<DirectX12Device> device,
                       std::shared_ptr<ScreenQuadMaterial> material)
    : device_(std::move(device)), material_(std::move(material)) {}

ScreenQuadMaterial::ScreenQuadMaterial(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

bool ScreenQuad::Initialize(UINT screen_width, UINT screen_height,
                            UINT bitmap_width, UINT bitmap_height) {

  screen_width_ = screen_width;
  screen_height_ = screen_height;

  quad_width_ = bitmap_width;
  quad_height_ = bitmap_height;

  if (!material_) {
    OutputDebugStringW(L"[ScreenQuad] Initialize failed: material is null.\n");
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

const VertexBufferView &ScreenQuad::GetVertexBufferView() const {
  return vertex_buffer_view_;
}

const IndexBufferView &ScreenQuad::GetIndexBufferView() const {
  return index_buffer_view_;
}

void ScreenQuad::SetVertexBufferView(const VertexBufferView &view) {
  vertex_buffer_view_ = view;
}

void ScreenQuad::SetIndexBufferView(const IndexBufferView &view) {
  index_buffer_view_ = view;
  index_count_ =
      static_cast<UINT>(index_buffer_view_.SizeInBytes / sizeof(uint16_t));
}

ScreenQuadMaterial *ScreenQuad::GetMaterial() { return material_.get(); }

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
  UINT8 *data_begin = 0;
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

const int ScreenQuad::GetIndexCount() { return index_count_; }

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

  ResourceSharedPtr upload_index_buffer = nullptr;
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * index_count_),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&upload_index_buffer)))) {
    return false;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * index_count_),
          D3D12_RESOURCE_STATE_COMMON, nullptr,
          IID_PPV_ARGS(&index_buffer_)))) {
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
  init_data.SlicePitch = sizeof(uint16_t) * index_count_;

  if (FAILED(command_allocator->Reset())) {
    return false;
  }

  if (FAILED(command_list->Reset(command_allocator.Get(), nullptr))) {
    return false;
  }

  auto to_copy = CD3DX12_RESOURCE_BARRIER::Transition(
      index_buffer_.Get(), D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  command_list->ResourceBarrier(1, &to_copy);

  UpdateSubresources(command_list.Get(), index_buffer_.Get(),
                     upload_index_buffer.Get(), 0, 0, 1, &init_data);

  auto to_index = CD3DX12_RESOURCE_BARRIER::Transition(
      index_buffer_.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_INDEX_BUFFER);
  command_list->ResourceBarrier(1, &to_index);

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

  if (FAILED(command_queue->Signal(fence.Get(), 2))) {
    return false;
  }

  if (fence->GetCompletedValue() < 2) {
    if (FAILED(fence->SetEventOnCompletion(1, fence_event))) {
      return false;
    }
    WaitForSingleObject(fence_event, INFINITE);
  }

  CloseHandle(fence_event);

  index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
  index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;
  index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

  delete[] indices;
  indices = nullptr;

  return true;
}

ScreenQuadMaterial::~ScreenQuadMaterial() {}

ResourceSharedPtr ScreenQuadMaterial::GetConstantBuffer() const {
  return constant_buffer_;
}

bool ScreenQuadMaterial::UpdateConstantBuffer(const XMMATRIX &world,
                                              const XMMATRIX &view,
                                              const XMMATRIX &orthogonality) {

  UINT8 *data_begin = 0;
  if (FAILED(constant_buffer_->Map(0, nullptr,
                                   reinterpret_cast<void **>(&data_begin)))) {
    return false;
  } else {
    XMStoreFloat4x4(&matrix_constant_data_.world_, world);
    XMStoreFloat4x4(&matrix_constant_data_.view_, view);
    XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, orthogonality);
    memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
    constant_buffer_->Unmap(0, nullptr);
  }
  return true;
}

bool ScreenQuadMaterial::Initialize() {

  if (initialized_) {
    return true;
  }

  D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
  cbv_heap_desc.NumDescriptors = 1;
  cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

  auto device = device_->GetD3d12Device();
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(MatrixBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&constant_buffer_)))) {
    return false;
  }

  ZeroMemory(&matrix_constant_data_, sizeof(MatrixBufferType));

  if (!InitializeRootSignature()) {
    return false;
  }

  if (!InitializeGraphicsPipelineState()) {
    return false;
  }
  initialized_ = true;
  return true;
}

void ScreenQuadMaterial::SetExternalConstantBuffer(
    const ResourceSharedPtr &constant_buffer,
    const DirectX::XMMATRIX &initial_world,
    const DirectX::XMMATRIX &initial_view,
    const DirectX::XMMATRIX &initial_ortho) {
  constant_buffer_ = constant_buffer;
  XMStoreFloat4x4(&matrix_constant_data_.world_, initial_world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, initial_view);
  XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, initial_ortho);
}

bool ScreenQuadMaterial::InitializeGraphicsPipelineState() {

  D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.InputLayout = {input_element_descs, _countof(input_element_descs)};
  pso_desc.pRootSignature = GetRootSignature().Get();
  pso_desc.VS = GetVSByteCode();
  pso_desc.PS = GetPSByteCode();
  pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  pso_desc.SampleDesc.Count = 1;

  auto device = device_->GetD3d12Device();

  PipelineStateObjectPtr pso = {};
  if (FAILED(
          device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }
  SetPSOByName("bitmap_normal", pso);

  PipelineStateObjectPtr pso2 = {};
  pso_desc.DepthStencilState.DepthEnable = false;
  if (FAILED(device->CreateGraphicsPipelineState(&pso_desc,
                                                 IID_PPV_ARGS(&pso2)))) {
    return false;
  }
  SetPSOByName("bitmap_depth_disable", pso2);

  return true;
}

bool ScreenQuadMaterial::InitializeRootSignature() {

  CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
  descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[2];
  root_parameters[0].InitAsDescriptorTable(1, &descriptor_ranges_srv[0],
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(0, 0,
                                              D3D12_SHADER_VISIBILITY_VERTEX);

  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.MipLODBias = 0;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc.ShaderRegister = 0;
  sampler_desc.RegisterSpace = 0;
  sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  CD3DX12_ROOT_SIGNATURE_DESC rootsignature_Layout(
      ARRAYSIZE(root_parameters), root_parameters, 1, &sampler_desc,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

  ID3DBlob *signature_blob = nullptr;
  ID3DBlob *signature_error = nullptr;
  if (FAILED(D3D12SerializeRootSignature(&rootsignature_Layout,
                                         D3D_ROOT_SIGNATURE_VERSION_1,
                                         &signature_blob, &signature_error))) {
    return false;
  }

  RootSignaturePtr root_signature = {};
  if (FAILED(device_->GetD3d12Device()->CreateRootSignature(
          0, signature_blob->GetBufferPointer(),
          signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)))) {
    return false;
  }
  SetRootSignature(root_signature);

  return true;
}
