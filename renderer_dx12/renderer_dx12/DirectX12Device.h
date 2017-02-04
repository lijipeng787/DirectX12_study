#ifndef D3D12CLASS_H
#define D3D12CLASS_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <new>

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include <d3dcommon.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <DirectXMath.h>

#include "dds.h"
#include "DDSTextureLoader.h"
#include "d3dx12.h"

#define GRAPHICS_PSO_DESC D3D12_GRAPHICS_PIPELINE_STATE_DESC 

#define ROOT_SIGNATURE_DESC D3D12_ROOT_SIGNATURE_DESC

#define CHECK(return_value) ((return_value==false)?true:false)

#define CBSIZE(constant_buffer) ((sizeof(constant_buffer) + 255) & ~255)

typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeapPtr;

typedef std::vector<D3D12_VIEWPORT> ViewPortVector;

typedef std::vector<D3D12_RECT> ScissorRectVector;

typedef Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChainPtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugDevice> D2d12DebugDevicePtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugCommandQueue> D3d12DebugCommandQueuePtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugCommandList> D3d12DebugCommandListPtr;

typedef Microsoft::WRL::ComPtr<ID3D12Device> D3d12DevicePtr;
		
typedef Microsoft::WRL::ComPtr<ID3D12Resource> ResourceSharedPtr;
		
typedef Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocatorPtr;

typedef Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueuePtr;
		
typedef Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignaturePtr;
		
typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObjectPtr;

typedef Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandListPtr;
		
typedef Microsoft::WRL::ComPtr<ID3D12Fence> FencePtr;

class DirectX12Device {
public:
	DirectX12Device() {}

	DirectX12Device(const DirectX12Device& rhs) = delete;

	DirectX12Device& operator=(const DirectX12Device& rhs) = delete;

	~DirectX12Device() { WaitForPreviousFrame(); }
public:
	static DirectX12Device* GetD3d12DeviceInstance();

	inline D3d12DevicePtr& GetD3d12Device() { return d3d12device_; }
public:
	bool Initialize(
		int screen_width, int screen_height,
		bool vsync, HWND hwnd, bool fullscreen,
		float screen_depth, float screen_near
	);

	bool ExecuteDefaultGraphicsCommandList();

	bool ResetCommandList();

	bool CloseCommandList();

	bool ResetCommandAllocator();

	void SetGraphicsRootSignature(RootSignaturePtr& graphics_rootsignature);

	void SetPipelineStateObject(PipelineStateObjectPtr pso);

	void SetDescriptorHeaps(UINT num_descriptors, ID3D12DescriptorHeap** descriptor_arr);

	void SetGraphicsRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

	void SetGraphicsRootConstantBufferView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	void BindVertexBuffer(UINT start_slot, UINT num_views, const D3D12_VERTEX_BUFFER_VIEW* vertex_buffer);

	void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* index_buffer_view);

	void DirectX12Device::BeginDrawToOffScreen();

	void DirectX12Device::EndDrawToOffScreen();

	void DirectX12Device::BeginPopulateGraphicsCommandList();

	void DirectX12Device::EndPopulateGraphicsCommandList();

	void DirectX12Device::Draw(
		UINT IndexCountPerInstance,
		UINT InstanceCount = 1,
		UINT StartIndexLocation = 0,
		INT BaseVertexLocation = 0,
		UINT StartInstanceLocation = 0
	);

	bool WaitForPreviousFrame();

	DescriptorHeapPtr& GetOffScreenTextureHeapView() { return off_screen_srv_; }

	GraphicsCommandListPtr& GetDefaultGraphicsCommandList() { return default_graphics_command_list_; }

	GraphicsCommandListPtr& GetDefaultcopyCommandList() { return default_copy_command_list_; }

	CommandQueuePtr& GetDefaultGraphicsCommandQueeue() { return default_graphics_command_queue_; }

	CommandQueuePtr& GetDefaultCopyCommandQueeue() { return default_copy_command_queue_; }

	CommandAllocatorPtr& GetDefaultGraphicsCommandAllocator() { return default_graphics_command_allocator_; }

	CommandAllocatorPtr& GetDefaultCopyCommandAllocator() { return default_copy_command_allocator_; }

	void inline GetProjectionMatrix(DirectX::XMMATRIX& projection) { projection = projection_matrix_; }

	void inline GetWorldMatrix(DirectX::XMMATRIX& world) { world = world_matrix_; }

	void inline GetOrthoMatrix(DirectX::XMMATRIX& ortho) { ortho = ortho_matrix_; }

	void inline GetVideoCardInfo(char* card_name, int memory);
private:
	static const UINT frame_cout_ = 2;

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

	CommandAllocatorPtr default_graphics_command_allocator_ = nullptr;

	CommandAllocatorPtr default_copy_command_allocator_ = nullptr;

	GraphicsCommandListPtr default_graphics_command_list_ = nullptr;

	GraphicsCommandListPtr default_copy_command_list_ = nullptr;

	CommandQueuePtr default_graphics_command_queue_ = nullptr;

	CommandQueuePtr default_copy_command_queue_ = nullptr;

	FencePtr fence_ = nullptr;
private:
	ResourceSharedPtr off_screen_texture_ = { nullptr };

	DescriptorHeapPtr off_screen_srv_ = { nullptr };

	DescriptorHeapPtr off_screen_rtv_ = { nullptr };

	ResourceSharedPtr render_targets_[frame_cout_] = { nullptr };

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
private:
	static DirectX12Device* d3d12device_instance_;
};

#endif