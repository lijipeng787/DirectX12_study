#include "stdafx.h"

#include "DirectX12Device.h"

#include <sstream>

DirectX12Device::~DirectX12Device() {
  if (fence_ && default_graphics_command_queue_) {
    WaitForPreviousFrame();
  }
  if (fence_handle_) {
    CloseHandle(fence_handle_);
    fence_handle_ = nullptr;
  }
}

std::shared_ptr<DirectX12Device>
DirectX12Device::Create(const DirectX12DeviceConfig &config) {
  auto device = std::shared_ptr<DirectX12Device>(new DirectX12Device());
  if (!device->Initialize(config)) {
    return nullptr;
  }
  return device;
}

bool DirectX12Device::Initialize(const DirectX12DeviceConfig &config) {

  ResetDeviceState();

  config_ = config;
  is_vsync_enabled_ = config.vsync_enabled;

  HRESULT hr = EnableDebugLayer();
  if (FAILED(hr)) {
    LogInitializationFailure(L"EnableDebugLayer", hr);
    return false;
  }

  Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
  hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateDXGIFactory1", hr);
    return false;
  }

  dxgi_resources_ =
      std::make_unique<DxgiResourceManager>(Microsoft::WRL::ComPtr<IDXGIFactory4>(factory));

  hr = dxgi_resources_->CreateDevice(d3d12device_, video_card_memory_,
                                     video_card_description_);
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateDevice", hr);
    ResetDeviceState();
    return false;
  }

  hr = CreateCommandQueues();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateCommandQueues", hr);
    ResetDeviceState();
    return false;
  }

  hr = dxgi_resources_->CreateSwapChain(config_,
                                        default_graphics_command_queue_.Get(),
                                        swap_chain_, frame_cout_);
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateSwapChain", hr);
    ResetDeviceState();
    return false;
  }

  frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

  hr = CreateRenderTargetViews();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateRenderTargetViews", hr);
    ResetDeviceState();
    return false;
  }

  hr = CreateDepthStencilResources();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateDepthStencilResources", hr);
    ResetDeviceState();
    return false;
  }

  hr = CreateOffscreenResources();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateOffscreenResources", hr);
    ResetDeviceState();
    return false;
  }

  hr = CreateCommandAllocatorsAndLists();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateCommandAllocatorsAndLists", hr);
    ResetDeviceState();
    return false;
  }

  hr = CreateFenceAndEvent();
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateFenceAndEvent", hr);
    ResetDeviceState();
    return false;
  }

  InitializeViewportsAndScissors();
  InitializeMatrices();

  return true;
}

HRESULT DirectX12Device::EnableDebugLayer() {
#if defined(_DEBUG)
  Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
  HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller));
  if (FAILED(hr)) {
    return hr;
  }
  debug_controller->EnableDebugLayer();
#endif
  return S_OK;
}

HRESULT DirectX12Device::CreateCommandQueues() {
  if (!d3d12device_) {
    return E_FAIL;
  }

  default_graphics_command_queue_.Reset();
  default_copy_command_queue_.Reset();

  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

  HRESULT hr = d3d12device_->CreateCommandQueue(
      &queue_desc, IID_PPV_ARGS(&default_graphics_command_queue_));
  if (FAILED(hr)) {
    return hr;
  }

  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  hr = d3d12device_->CreateCommandQueue(
      &queue_desc, IID_PPV_ARGS(&default_copy_command_queue_));
  return hr;
}

HRESULT DirectX12Device::CreateRenderTargetViews() {
  if (!d3d12device_ || !swap_chain_) {
    return E_FAIL;
  }

  render_target_view_heap_.Reset();
  for (auto &render_target : back_buffer_render_targets_) {
    render_target.Reset();
  }

  D3D12_DESCRIPTOR_HEAP_DESC render_target_heap_desc = {};
  render_target_heap_desc.NumDescriptors = frame_cout_;
  render_target_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  render_target_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  HRESULT hr = d3d12device_->CreateDescriptorHeap(
      &render_target_heap_desc, IID_PPV_ARGS(&render_target_view_heap_));
  if (FAILED(hr)) {
    return hr;
  }

  render_target_descriptor_size_ =
      d3d12device_->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  CD3DX12_CPU_DESCRIPTOR_HANDLE render_target_handle(
      render_target_view_heap_->GetCPUDescriptorHandleForHeapStart());

  for (UINT index = 0; index < frame_cout_; ++index) {
    hr = swap_chain_->GetBuffer(index,
                                IID_PPV_ARGS(&back_buffer_render_targets_[index]));
    if (FAILED(hr)) {
      return hr;
    }
    d3d12device_->CreateRenderTargetView(
        back_buffer_render_targets_[index].Get(), nullptr,
                                         render_target_handle);
    render_target_handle.Offset(1, render_target_descriptor_size_);
  }

  return S_OK;
}

HRESULT DirectX12Device::CreateDepthStencilResources() {
  if (!d3d12device_) {
    return E_FAIL;
  }

  depth_stencil_view_heap_.Reset();
  depth_stencil_resource_.Reset();

  D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
  dsv_heap_desc.NumDescriptors = 1;
  dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
  dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

  HRESULT hr = d3d12device_->CreateDescriptorHeap(
      &dsv_heap_desc, IID_PPV_ARGS(&depth_stencil_view_heap_));
  if (FAILED(hr)) {
    return hr;
  }

  D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_desc = {};
  depth_stencil_desc.Format = DXGI_FORMAT_D32_FLOAT;
  depth_stencil_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
  depth_stencil_desc.Flags = D3D12_DSV_FLAG_NONE;

  D3D12_CLEAR_VALUE depth_clear_value = {};
  depth_clear_value.Format = DXGI_FORMAT_D32_FLOAT;
  depth_clear_value.DepthStencil.Depth = 1.0f;
  depth_clear_value.DepthStencil.Stencil = 0;

  hr = d3d12device_->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE,
      &CD3DX12_RESOURCE_DESC::Tex2D(
          DXGI_FORMAT_D32_FLOAT, config_.screen_width, config_.screen_height, 1,
          0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
      D3D12_RESOURCE_STATE_DEPTH_WRITE, &depth_clear_value,
      IID_PPV_ARGS(&depth_stencil_resource_));
  if (FAILED(hr)) {
    return hr;
  }

  d3d12device_->CreateDepthStencilView(
      depth_stencil_resource_.Get(), &depth_stencil_desc,
      depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart());

  depth_stencil_descriptor_size_ =
      d3d12device_->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

  return S_OK;
}

HRESULT DirectX12Device::CreateOffscreenResources() {
  if (!d3d12device_) {
    return E_FAIL;
  }

  if (default_offscreen_handle_ != kInvalidRenderTargetHandle) {
    DestroyRenderTarget(default_offscreen_handle_);
    default_offscreen_handle_ = kInvalidRenderTargetHandle;
  }

  RenderTargetDescriptor descriptor = {};
  descriptor.width = config_.screen_width;
  descriptor.height = config_.screen_height;
  descriptor.format = DXGI_FORMAT_R8G8B8A8_UNORM;
  descriptor.create_rtv = true;
  descriptor.create_srv = true;
  descriptor.clear_color[0] = 0.0f;
  descriptor.clear_color[1] = 0.2f;
  descriptor.clear_color[2] = 0.4f;
  descriptor.clear_color[3] = 1.0f;

  auto handle = CreateRenderTarget(descriptor);
  if (handle == kInvalidRenderTargetHandle) {
    return E_FAIL;
  }

  default_offscreen_handle_ = handle;
  return S_OK;
}

HRESULT DirectX12Device::CreateCommandAllocatorsAndLists() {
  if (!d3d12device_) {
    return E_FAIL;
  }

  if (frame_resources_.empty()) {
    frame_resources_.resize(frame_cout_);
  }

  for (auto &frame : frame_resources_) {
    frame.command_allocator.Reset();
    frame.fence_value = 0;
  }

  default_copy_command_allocator_.Reset();
  default_graphics_command_list_.Reset();
  default_copy_command_list_.Reset();

  HRESULT hr = S_OK;

  for (auto &frame : frame_resources_) {
    hr = d3d12device_->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&frame.command_allocator));
    if (FAILED(hr)) {
      return hr;
    }
  }

  hr = d3d12device_->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_COPY,
      IID_PPV_ARGS(&default_copy_command_allocator_));
  if (FAILED(hr)) {
    return hr;
  }

  hr = d3d12device_->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_DIRECT,
      frame_resources_[frame_index_].command_allocator.Get(), nullptr,
      IID_PPV_ARGS(&default_graphics_command_list_));
  if (FAILED(hr)) {
    return hr;
  }

  hr = d3d12device_->CreateCommandList(
      0, D3D12_COMMAND_LIST_TYPE_COPY,
      default_copy_command_allocator_.Get(), nullptr,
      IID_PPV_ARGS(&default_copy_command_list_));
  if (FAILED(hr)) {
    return hr;
  }

  hr = default_graphics_command_list_->Close();
  if (FAILED(hr)) {
    return hr;
  }

  return default_copy_command_list_->Close();
}

HRESULT DirectX12Device::CreateFenceAndEvent() {
  if (!d3d12device_) {
    return E_FAIL;
  }

  HRESULT hr = d3d12device_->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                         IID_PPV_ARGS(&fence_));
  if (FAILED(hr)) {
    return hr;
  }

  fence_handle_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!fence_handle_) {
    return HRESULT_FROM_WIN32(GetLastError());
  }

  return S_OK;
}

auto DirectX12Device::CreateRenderTarget(const RenderTargetDescriptor &descriptor) -> RenderTargetHandle {
  if (!d3d12device_) {
    LogInitializationFailure(L"CreateRenderTarget::DeviceMissing", E_FAIL);
    return kInvalidRenderTargetHandle;
  }

  RenderTargetResource resource = {};
  resource.descriptor = descriptor;

  D3D12_RESOURCE_DESC texture_desc = {};
  texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  texture_desc.Alignment = 0;
  texture_desc.Width = descriptor.width;
  texture_desc.Height = descriptor.height;
  texture_desc.DepthOrArraySize = 1;
  texture_desc.MipLevels = 1;
  texture_desc.Format = descriptor.format;
  texture_desc.SampleDesc.Count = 1;
  texture_desc.SampleDesc.Quality = 0;
  texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  texture_desc.Flags = descriptor.resource_flags;

  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = descriptor.format;
  clear_value.Color[0] = descriptor.clear_color[0];
  clear_value.Color[1] = descriptor.clear_color[1];
  clear_value.Color[2] = descriptor.clear_color[2];
  clear_value.Color[3] = descriptor.clear_color[3];

  HRESULT hr = d3d12device_->CreateCommittedResource(
      &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
      D3D12_HEAP_FLAG_NONE, &texture_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, &clear_value,
      IID_PPV_ARGS(&resource.texture));
  if (FAILED(hr)) {
    LogInitializationFailure(L"CreateRenderTarget::CreateTexture", hr);
    return kInvalidRenderTargetHandle;
  }

  resource.current_state = D3D12_RESOURCE_STATE_GENERIC_READ;

  if (descriptor.create_srv) {
    D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
    srv_heap_desc.NumDescriptors = 1;
    srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = d3d12device_->CreateDescriptorHeap(
        &srv_heap_desc, IID_PPV_ARGS(&resource.srv));
    if (FAILED(hr)) {
      LogInitializationFailure(L"CreateRenderTarget::CreateSrvHeap", hr);
      return kInvalidRenderTargetHandle;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = descriptor.format;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

    d3d12device_->CreateShaderResourceView(
        resource.texture.Get(), &srv_desc,
        resource.srv->GetCPUDescriptorHandleForHeapStart());
  }

  if (descriptor.create_rtv) {
    D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
    rtv_heap_desc.NumDescriptors = 1;
    rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    hr = d3d12device_->CreateDescriptorHeap(
        &rtv_heap_desc, IID_PPV_ARGS(&resource.rtv));
    if (FAILED(hr)) {
      LogInitializationFailure(L"CreateRenderTarget::CreateRtvHeap", hr);
      return kInvalidRenderTargetHandle;
    }

    D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
    rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Format = descriptor.format;
    rtv_desc.Texture2D.MipSlice = 0;
    rtv_desc.Texture2D.PlaneSlice = 0;

    d3d12device_->CreateRenderTargetView(
        resource.texture.Get(), &rtv_desc,
        resource.rtv->GetCPUDescriptorHandleForHeapStart());
  }

  RenderTargetHandle handle = next_render_target_handle_++;
  user_render_targets_.insert(std::make_pair(handle, std::move(resource)));

  std::wstringstream stream;
  stream << L"[DirectX12Device] Created render target handle " << handle << L" ("
         << descriptor.width << L"x" << descriptor.height << L").\n";
  OutputDebugStringW(stream.str().c_str());

  return handle;
}

void DirectX12Device::DestroyRenderTarget(RenderTargetHandle handle) {
  if (handle == kInvalidRenderTargetHandle) {
    return;
  }

  auto it = user_render_targets_.find(handle);
  if (it == user_render_targets_.end()) {
    return;
  }

  if (default_offscreen_handle_ == handle) {
    default_offscreen_handle_ = kInvalidRenderTargetHandle;
  }

  user_render_targets_.erase(it);
}

auto DirectX12Device::GetRenderTargetSrv(RenderTargetHandle handle) const -> DescriptorHeapPtr {
  auto resolved = ResolveRenderTargetHandle(handle);
  auto it = user_render_targets_.find(resolved);
  if (it == user_render_targets_.end()) {
    return nullptr;
  }
  return it->second.srv;
}

auto DirectX12Device::ResolveRenderTargetHandle(RenderTargetHandle handle) const -> RenderTargetHandle {
  if (handle == kInvalidRenderTargetHandle) {
    return default_offscreen_handle_;
  }
  auto it = user_render_targets_.find(handle);
  if (it == user_render_targets_.end()) {
    return kInvalidRenderTargetHandle;
  }
  return handle;
}

auto DirectX12Device::GetRenderTargetResource(RenderTargetHandle handle) -> DirectX12Device::RenderTargetResource * {
  auto resolved = ResolveRenderTargetHandle(handle);
  if (resolved == kInvalidRenderTargetHandle) {
    return nullptr;
  }
  auto it = user_render_targets_.find(resolved);
  if (it == user_render_targets_.end()) {
    return nullptr;
  }
  return &it->second;
}

auto DirectX12Device::GetRenderTargetResource(RenderTargetHandle handle) const -> const DirectX12Device::RenderTargetResource * {
  auto resolved = ResolveRenderTargetHandle(handle);
  if (resolved == kInvalidRenderTargetHandle) {
    return nullptr;
  }
  auto it = user_render_targets_.find(resolved);
  if (it == user_render_targets_.end()) {
    return nullptr;
  }
  return &it->second;
}

void DirectX12Device::InitializeViewportsAndScissors() {
  viewport_.clear();
  scissor_rect_.clear();

  D3D12_VIEWPORT view_port = {};
  view_port.Width = static_cast<float>(config_.screen_width);
  view_port.Height = static_cast<float>(config_.screen_height);
  view_port.MinDepth = 0.0f;
  view_port.MaxDepth = 1.0f;
  view_port.TopLeftX = 0.0f;
  view_port.TopLeftY = 0.0f;
  viewport_.push_back(view_port);

  D3D12_RECT rect = {};
  rect.left = 0;
  rect.top = 0;
  rect.right = static_cast<LONG>(config_.screen_width);
  rect.bottom = static_cast<LONG>(config_.screen_height);
  scissor_rect_.push_back(rect);
}

void DirectX12Device::InitializeMatrices() {
  const float field_of_view = DirectX::XM_PI / 4.0f;
  const float screen_aspect =
      static_cast<float>(config_.screen_width) /
      static_cast<float>(config_.screen_height);

  projection_matrix_ = DirectX::XMMatrixPerspectiveFovLH(
      field_of_view, screen_aspect, config_.screen_near, config_.screen_depth);

  world_matrix_ = DirectX::XMMatrixIdentity();

  ortho_matrix_ = DirectX::XMMatrixOrthographicLH(
      static_cast<float>(config_.screen_width),
      static_cast<float>(config_.screen_height), config_.screen_near,
      config_.screen_depth);
}

bool DirectX12Device::ExecuteDefaultGraphicsCommandList() {

  if (FAILED(default_graphics_command_list_->Close())) {
    return false;
  }

  ID3D12CommandList *command_list[] = {default_graphics_command_list_.Get()};
  default_graphics_command_queue_->ExecuteCommandLists(1, command_list);

  if (FAILED(swap_chain_->Present(1, 0))) {
    return false;
  }

  if (CHECK(WaitForPreviousFrame())) {
    return false;
  }

  return true;
}

bool DirectX12Device::ResetCommandList() {
  auto &frame = CurrentFrameResource();
  if (!frame.command_allocator) {
    return false;
  }
  if (FAILED(default_graphics_command_list_->Reset(
          frame.command_allocator.Get(), nullptr))) {
    return false;
  }
  return true;
}

bool DirectX12Device::CloseCommandList() {
  if (FAILED(default_graphics_command_list_->Close())) {
    return false;
  }
  return true;
}

bool DirectX12Device::ResetCommandAllocator() {
  auto &frame = CurrentFrameResource();
  if (!frame.command_allocator) {
    return false;
  }
  if (FAILED(frame.command_allocator->Reset())) {
    return false;
  }
  return true;
}

void DirectX12Device::SetGraphicsRootSignature(
    const RootSignaturePtr &graphics_rootsignature) {
  default_graphics_command_list_->SetGraphicsRootSignature(
      graphics_rootsignature.Get());
}

void DirectX12Device::SetPipelineStateObject(
    const PipelineStateObjectPtr &pso) {
  default_graphics_command_list_->SetPipelineState(pso.Get());
}

void DirectX12Device::SetDescriptorHeaps(
    UINT num_descriptors, ID3D12DescriptorHeap **descriptor_arr) {
  default_graphics_command_list_->SetDescriptorHeaps(num_descriptors,
                                                     descriptor_arr);
}

void DirectX12Device::SetGraphicsRootDescriptorTable(
    UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
  default_graphics_command_list_->SetGraphicsRootDescriptorTable(
      RootParameterIndex, BaseDescriptor);
}

void DirectX12Device::SetGraphicsRootConstantBufferView(
    UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
  default_graphics_command_list_->SetGraphicsRootConstantBufferView(
      RootParameterIndex, BufferLocation);
}

void DirectX12Device::BindVertexBuffer(UINT start_slot, UINT num_views,
                                       const VertexBufferView *vertex_buffer) {
  default_graphics_command_list_->IASetVertexBuffers(start_slot, num_views,
                                                     vertex_buffer);
}

void DirectX12Device::BindIndexBuffer(
    const IndexBufferView *index_buffer_view) {
  default_graphics_command_list_->IASetIndexBuffer(index_buffer_view);
}

void DirectX12Device::BeginDrawToOffScreen(RenderTargetHandle handle) {
  auto resource = GetRenderTargetResource(handle);
  if (!resource || !resource->texture || !resource->rtv) {
    return;
  }

  default_graphics_command_list_->RSSetViewports(1, &viewport_.at(0));
  default_graphics_command_list_->RSSetScissorRects(1, &scissor_rect_.at(0));

  if (resource->current_state != D3D12_RESOURCE_STATE_RENDER_TARGET) {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource->texture.Get(), resource->current_state,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    default_graphics_command_list_->ResourceBarrier(1, &barrier);
    resource->current_state = D3D12_RESOURCE_STATE_RENDER_TARGET;
  }

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
      resource->rtv->GetCPUDescriptorHandleForHeapStart());

  CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(
      depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart());

  default_graphics_command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE,
                                                     &dsv_handle);

  default_graphics_command_list_->ClearRenderTargetView(
      rtv_handle, resource->descriptor.clear_color, 0, nullptr);
  default_graphics_command_list_->ClearDepthStencilView(
      dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  default_graphics_command_list_->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DirectX12Device::EndDrawToOffScreen(RenderTargetHandle handle) {
  auto resource = GetRenderTargetResource(handle);
  if (!resource || !resource->texture) {
    return;
  }

  if (resource->current_state != D3D12_RESOURCE_STATE_GENERIC_READ) {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        resource->texture.Get(), resource->current_state,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    default_graphics_command_list_->ResourceBarrier(1, &barrier);
    resource->current_state = D3D12_RESOURCE_STATE_GENERIC_READ;
  }
}

void DirectX12Device::BeginPopulateGraphicsCommandList() {
  default_graphics_command_list_->RSSetViewports(1, &viewport_.at(0));
  default_graphics_command_list_->RSSetScissorRects(1, &scissor_rect_.at(0));

  default_graphics_command_list_->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(
             back_buffer_render_targets_[frame_index_].Get(),
             D3D12_RESOURCE_STATE_PRESENT,
             D3D12_RESOURCE_STATE_RENDER_TARGET));

  CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
      render_target_view_heap_->GetCPUDescriptorHandleForHeapStart(),
      frame_index_, render_target_descriptor_size_);

  CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(
      depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart());

  default_graphics_command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE,
                                                     &dsv_handle);

  const float clear_color[] = {0.0f, 0.2f, 0.4f, 1.0f};
  default_graphics_command_list_->ClearRenderTargetView(rtv_handle, clear_color,
                                                        0, nullptr);
  default_graphics_command_list_->ClearDepthStencilView(
      dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
  default_graphics_command_list_->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DirectX12Device::EndPopulateGraphicsCommandList() {
  default_graphics_command_list_->ResourceBarrier(
      1, &CD3DX12_RESOURCE_BARRIER::Transition(
             back_buffer_render_targets_[frame_index_].Get(),
             D3D12_RESOURCE_STATE_RENDER_TARGET,
             D3D12_RESOURCE_STATE_PRESENT));
}

void DirectX12Device::Draw(UINT IndexCountPerInstance, UINT InstanceCount,
                           UINT StartIndexLocation, INT BaseVertexLocation,
                           UINT StartInstanceLocation) {

  default_graphics_command_list_->DrawIndexedInstanced(
      IndexCountPerInstance, InstanceCount, StartIndexLocation,
      BaseVertexLocation, StartInstanceLocation);
}

bool DirectX12Device::WaitForPreviousFrame() {

  auto &current_frame = CurrentFrameResource();

  const UINT64 fence_to_wait = fence_value_;
  if (FAILED(default_graphics_command_queue_->Signal(fence_.Get(),
                                                     fence_to_wait))) {
    return false;
  }
  current_frame.fence_value = fence_to_wait;
  ++fence_value_;

  frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

  auto &next_frame = CurrentFrameResource();
  if (next_frame.fence_value != 0 &&
      fence_->GetCompletedValue() < next_frame.fence_value) {
    if (FAILED(
            fence_->SetEventOnCompletion(next_frame.fence_value, fence_handle_))) {
      return false;
    }
    WaitForSingleObject(fence_handle_, INFINITE);
  }

  return true;
}

void DirectX12Device::GetVideoCardInfo(char *card_name, int &memory) {
  strcpy_s(card_name, 128, video_card_description_);
  memory = video_card_memory_;
}

void DirectX12Device::ResetDeviceState() {
  for (auto &target : back_buffer_render_targets_) {
    target.Reset();
  }
  user_render_targets_.clear();
  default_offscreen_handle_ = kInvalidRenderTargetHandle;
  next_render_target_handle_ = 0;

  render_target_view_heap_.Reset();
  depth_stencil_resource_.Reset();
  depth_stencil_view_heap_.Reset();

  default_graphics_command_list_.Reset();
  default_copy_command_list_.Reset();
  default_copy_command_allocator_.Reset();
  frame_resources_.clear();

  default_graphics_command_queue_.Reset();
  default_copy_command_queue_.Reset();

  if (fence_handle_) {
    CloseHandle(fence_handle_);
    fence_handle_ = nullptr;
  }
  fence_.Reset();

  swap_chain_.Reset();
  dxgi_resources_.reset();
  d3d12device_.Reset();

  viewport_.clear();
  scissor_rect_.clear();

  projection_matrix_ = DirectX::XMMatrixIdentity();
  world_matrix_ = DirectX::XMMatrixIdentity();
  ortho_matrix_ = DirectX::XMMatrixIdentity();

  frame_index_ = 0;
  fence_value_ = 1;
}

void DirectX12Device::LogInitializationFailure(const wchar_t *stage,
                                               HRESULT hr) const {
  std::wstringstream stream;
  stream << L"[DirectX12Device] Initialization failed at " << stage
         << L" (hr=0x" << std::hex << hr << std::dec << L")\n";
  OutputDebugStringW(stream.str().c_str());
}

DirectX12Device::FrameResource &DirectX12Device::CurrentFrameResource() {
  return frame_resources_.at(frame_index_);
}

const DirectX12Device::FrameResource &
DirectX12Device::CurrentFrameResource() const {
  return frame_resources_.at(frame_index_);
}
