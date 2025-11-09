#ifndef D3D12CLASS_H
#define D3D12CLASS_H

#include <sstream>
#include <unordered_map>
#include <limits>
#include <utility>

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "TypeDefine.h"
#include "d3dx12.h"

struct DirectX12DeviceConfig {
    int screen_width = 0;
    int screen_height = 0;
    bool vsync_enabled = false;
    HWND hwnd = nullptr;
    bool fullscreen = false;
    float screen_depth = 1000.0f;
    float screen_near = 0.1f;
};

class DxgiResourceManager {
public:
    explicit DxgiResourceManager(Microsoft::WRL::ComPtr<IDXGIFactory4> factory)
        : factory_(std::move(factory)) {
    }

    HRESULT CreateDevice(D3d12DevicePtr& device, int& video_memory_mb,
        char(&description)[128]) {
        if (!factory_) {
            return E_FAIL;
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        DXGI_ADAPTER_DESC1 adapter_desc = {};

        for (UINT adapter_index = 0;
            factory_->EnumAdapters1(adapter_index,
                adapter.ReleaseAndGetAddressOf()) !=
            DXGI_ERROR_NOT_FOUND;
            ++adapter_index) {

            adapter->GetDesc1(&adapter_desc);
            if (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                LogAdapterAttempt(adapter_desc, DXGI_ERROR_UNSUPPORTED);
                continue;
            }

            HRESULT hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&device));
            LogAdapterAttempt(adapter_desc, hr);
            if (SUCCEEDED(hr)) {
                adapter_ = adapter;
                video_memory_mb =
                    static_cast<int>(adapter_desc.DedicatedVideoMemory / 1024 / 1024);
                size_t string_length = 0;
                errno_t error = wcstombs_s(&string_length, description, 128,
                    adapter_desc.Description, 128);
                if (error != 0) {
                    return E_FAIL;
                }
                return S_OK;
            }
            device.Reset();
        }

        OutputDebugStringW(
            L"[DxgiResourceManager] Can not find available hardware adapter, create device failed.\n");
        return DXGI_ERROR_NOT_FOUND;
    }

    HRESULT CreateSwapChain(const DirectX12DeviceConfig& config,
        ID3D12CommandQueue* command_queue,
        Microsoft::WRL::ComPtr<IDXGISwapChain3>& swap_chain,
        UINT frame_count) {
        if (!factory_ || !command_queue) {
            return E_INVALIDARG;
        }

        swap_chain.Reset();

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
        swap_chain_desc.BufferCount = frame_count;
        swap_chain_desc.Width = config.screen_width;
        swap_chain_desc.Height = config.screen_height;
        swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.SampleDesc.Count = 1;

        Microsoft::WRL::ComPtr<IDXGISwapChain1> temp_swap_chain;
        HRESULT hr = factory_->CreateSwapChainForHwnd(
            command_queue, config.hwnd, &swap_chain_desc, nullptr, nullptr,
            &temp_swap_chain);
        if (FAILED(hr)) {
            OutputDebugStringW(
                L"[DxgiResourceManager] Create SwapChain failed, CreateSwapChainForHwnd "
                L"returned error.\n");
            return hr;
        }

        hr = factory_->MakeWindowAssociation(config.hwnd, DXGI_MWA_NO_ALT_ENTER);
        if (FAILED(hr)) {
            OutputDebugStringW(
                L"[DxgiResourceManager] MakeWindowAssociation failed.\n");
            return hr;
        }

        hr = temp_swap_chain.As(&swap_chain);
        if (FAILED(hr)) {
            OutputDebugStringW(
                L"[DxgiResourceManager] Failed to cast SwapChain to IDXGISwapChain3.\n");
            return hr;
        }

        std::wstringstream stream;
        stream << L"[DxgiResourceManager] Create SwapChain successfully, size "
            << config.screen_width << L"x" << config.screen_height << L", buffer count "
            << frame_count << L".\n";
        OutputDebugStringW(stream.str().c_str()); 

        return S_OK;
    }

private:
    void LogAdapterAttempt(const DXGI_ADAPTER_DESC1& desc,
        HRESULT result) const {
        std::wstringstream stream;
        stream << L"[DxgiResourceManager] try adapter " << desc.Description
            << L", video memory "
            << static_cast<unsigned long long>(
                desc.DedicatedVideoMemory / (1024ull * 1024ull))
            << L" MB, result 0x" << std::hex << result << std::dec << L".\n";
        OutputDebugStringW(stream.str().c_str());
    }

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory_ = nullptr;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter_ = nullptr;
};

struct RenderTargetDescriptor {
  UINT width = 0;
  UINT height = 0;
  DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
  bool create_rtv = true;
  bool create_srv = true;
  float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
  D3D12_RESOURCE_FLAGS resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
};

using RenderTargetHandle = size_t;

static constexpr RenderTargetHandle kInvalidRenderTargetHandle =
    std::numeric_limits<RenderTargetHandle>::max();

class DirectX12Device : public std::enable_shared_from_this<DirectX12Device> {
public:
  DirectX12Device() {}

  DirectX12Device(const DirectX12Device &rhs) = delete;

  DirectX12Device &operator=(const DirectX12Device &rhs) = delete;

  ~DirectX12Device();

public:
  static std::shared_ptr<DirectX12Device>
  Create(const DirectX12DeviceConfig &config);

  inline D3d12DevicePtr GetD3d12Device() { return d3d12device_; }

public:
  bool Initialize(const DirectX12DeviceConfig &config);

  bool ExecuteDefaultGraphicsCommandList();

  bool ResetCommandList();

  bool CloseCommandList();

  bool ResetCommandAllocator();

  void SetGraphicsRootSignature(const RootSignaturePtr &graphics_rootsignature);

  void SetPipelineStateObject(const PipelineStateObjectPtr &pso);

  void SetDescriptorHeaps(UINT num_descriptors,
                          ID3D12DescriptorHeap **descriptor_arr);

  void
  SetGraphicsRootDescriptorTable(UINT RootParameterIndex,
                                 D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

  void
  SetGraphicsRootConstantBufferView(UINT RootParameterIndex,
                                    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

  void BindVertexBuffer(UINT start_slot, UINT num_views,
                        const VertexBufferView *vertex_buffer);

  void BindIndexBuffer(const IndexBufferView *index_buffer_view);

  void DirectX12Device::BeginDrawToOffScreen(
      RenderTargetHandle handle = kInvalidRenderTargetHandle);

  void DirectX12Device::EndDrawToOffScreen(
      RenderTargetHandle handle = kInvalidRenderTargetHandle);

  void DirectX12Device::BeginPopulateGraphicsCommandList();

  void DirectX12Device::EndPopulateGraphicsCommandList();

  void DirectX12Device::Draw(UINT IndexCountPerInstance, UINT InstanceCount = 1,
                             UINT StartIndexLocation = 0,
                             INT BaseVertexLocation = 0,
                             UINT StartInstanceLocation = 0);

  bool WaitForPreviousFrame();

  DescriptorHeapPtr GetOffScreenTextureHeapView() {
    return GetRenderTargetSrv(default_offscreen_handle_);
  }

  GraphicsCommandListPtr GetDefaultGraphicsCommandList() {
    return default_graphics_command_list_;
  }

  GraphicsCommandListPtr GetDefaultcopyCommandList() {
    return default_copy_command_list_;
  }

  CommandQueuePtr GetDefaultGraphicsCommandQueeue() {
    return default_graphics_command_queue_;
  }

  CommandQueuePtr GetDefaultCopyCommandQueeue() {
    return default_copy_command_queue_;
  }

  CommandAllocatorPtr GetDefaultGraphicsCommandAllocator() {
    if (frame_resources_.empty()) {
      return {};
    }
    return frame_resources_[frame_index_].command_allocator;
  }

  CommandAllocatorPtr GetDefaultCopyCommandAllocator() {
    return default_copy_command_allocator_;
  }

  void inline GetProjectionMatrix(DirectX::XMMATRIX &projection) {
    projection = projection_matrix_;
  }

  void inline GetWorldMatrix(DirectX::XMMATRIX &world) {
    world = world_matrix_;
  }

  void inline GetOrthoMatrix(DirectX::XMMATRIX &ortho) {
    ortho = ortho_matrix_;
  }

  void inline GetVideoCardInfo(char *card_name, int &memory);

  RenderTargetHandle CreateRenderTarget(const RenderTargetDescriptor &descriptor);

  void DestroyRenderTarget(RenderTargetHandle handle);

  DescriptorHeapPtr GetRenderTargetSrv(RenderTargetHandle handle) const;

private:
  HRESULT EnableDebugLayer();

  HRESULT CreateCommandQueues();

  HRESULT CreateRenderTargetViews();

  HRESULT CreateDepthStencilResources();

  HRESULT CreateOffscreenResources();

  HRESULT CreateCommandAllocatorsAndLists();

  HRESULT CreateFenceAndEvent();

  void InitializeViewportsAndScissors();

  void InitializeMatrices();

  void ResetDeviceState();

  void LogInitializationFailure(const wchar_t *stage, HRESULT hr) const;

private:
  static const UINT frame_cout_ = 2;

  DirectX12DeviceConfig config_ = {};

  bool is_vsync_enabled_ = false;

  int video_card_memory_ = 0;

  char video_card_description_[128] = {};

private:
  ViewPortVector viewport_ = {};

  ScissorRectVector scissor_rect_ = {};

  SwapChainPtr swap_chain_ = nullptr;

  D2d12DebugDevicePtr d3d12_debug_device_ = nullptr;

  D3d12DebugCommandQueuePtr debug_command_queue_ = nullptr;

  D3d12DebugCommandListPtr debug_command_list_ = nullptr;

  D3d12DevicePtr d3d12device_ = nullptr;

  CommandAllocatorPtr default_copy_command_allocator_ = nullptr;

  GraphicsCommandListPtr default_graphics_command_list_ = nullptr;

  GraphicsCommandListPtr default_copy_command_list_ = nullptr;

  CommandQueuePtr default_graphics_command_queue_ = nullptr;

  CommandQueuePtr default_copy_command_queue_ = nullptr;

  FencePtr fence_ = nullptr;

private:
  ResourceSharedPtr back_buffer_render_targets_[frame_cout_] = {nullptr};

  struct RenderTargetResource {
    RenderTargetDescriptor descriptor = {};
    ResourceSharedPtr texture = nullptr;
    DescriptorHeapPtr srv = nullptr;
    DescriptorHeapPtr rtv = nullptr;
    D3D12_RESOURCE_STATES current_state = D3D12_RESOURCE_STATE_COMMON;
  };

  DescriptorHeapPtr render_target_view_heap_ = nullptr;

  UINT render_target_descriptor_size_ = 0;

  ResourceSharedPtr depth_stencil_resource_ = nullptr;

  DescriptorHeapPtr depth_stencil_view_heap_ = nullptr;

  UINT depth_stencil_descriptor_size_ = 0;

  RootSignaturePtr root_signature_ = nullptr;

  PipelineStateObjectPtr pso_ = nullptr;

  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT frame_index_ = 0;

  HANDLE fence_handle_ = nullptr;

  UINT64 fence_value_ = 1;

private:
  DirectX::XMMATRIX projection_matrix_ = {};

  DirectX::XMMATRIX world_matrix_ = {};

  DirectX::XMMATRIX ortho_matrix_ = {};

  struct FrameResource {
    CommandAllocatorPtr command_allocator = nullptr;
    UINT64 fence_value = 0;
  };

  std::vector<FrameResource> frame_resources_ = {};

  std::unique_ptr<DxgiResourceManager> dxgi_resources_ = nullptr;

  std::unordered_map<RenderTargetHandle, RenderTargetResource> user_render_targets_ =
      {};

  RenderTargetHandle default_offscreen_handle_ = kInvalidRenderTargetHandle;

  RenderTargetHandle next_render_target_handle_ = 0;

  RenderTargetResource *GetRenderTargetResource(RenderTargetHandle handle);

  const RenderTargetResource *
  GetRenderTargetResource(RenderTargetHandle handle) const;

  RenderTargetHandle ResolveRenderTargetHandle(RenderTargetHandle handle) const;

  FrameResource &CurrentFrameResource();
  const FrameResource &CurrentFrameResource() const;
};

#endif