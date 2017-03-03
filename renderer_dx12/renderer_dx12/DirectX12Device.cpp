#include "stdafx.h"

#include "DirectX12Device.h"

DirectX12Device* DirectX12Device::d3d12device_instance_ = nullptr;

DirectX12Device* DirectX12Device::GetD3d12DeviceInstance(){
	
	if (nullptr == d3d12device_instance_) {
		d3d12device_instance_ = new DirectX12Device;
	}
	return d3d12device_instance_;
}

bool DirectX12Device::Initialize(
	int screen_width, int screen_height,
	bool vsync,HWND hwnd,bool fullscreen,
	float screen_depth, float screen_near
) {

#if defined (_DEBUG)

	Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller = nullptr;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
		debug_controller->EnableDebugLayer();
	}
	else {
		assert(0);
	}

#endif // !(_DEBUG)

	IDXGIFactory4 *factory = nullptr;
	{
		if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
			return false;
		}
	}

	IDXGIAdapter1 *adapter = nullptr;
	DXGI_ADAPTER_DESC1 adapter_desc = { 0 };

	for (UINT adapter_index = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapter_index, &adapter); ++adapter_index) {
		adapter->GetDesc1(&adapter_desc);
		if (DXGI_ADAPTER_FLAG_SOFTWARE&adapter_desc.Flags) {
			continue;
		}
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12device_)))) {

			video_card_memory_ = static_cast<int>(adapter_desc.DedicatedVideoMemory / 1024 / 1024);
			size_t string_length = 0;
			auto error = wcstombs_s(&string_length, video_card_description_, 128, adapter_desc.Description, 128);
			if (error != 0) {
				return false;
			}

			break;
		}
		adapter_desc = { 0 };
	}
	adapter->Release();

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if (FAILED(d3d12device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&default_graphics_command_queue_)))) {
		return false;
	}

	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	if (FAILED(d3d12device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&default_copy_command_queue_)))) {
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = frame_cout_;
	swap_chain_desc.Width = screen_width;
	swap_chain_desc.Height = screen_height;
	swap_chain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> tem_swap_chain = nullptr;

	if (FAILED(factory->CreateSwapChainForHwnd(
		default_graphics_command_queue_.Get(),
		hwnd,
		&swap_chain_desc,
		nullptr,
		nullptr,
		&tem_swap_chain
	))) {
		return false;
	}

	if (FAILED(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER))) {
		return false;
	}

	if (FAILED(tem_swap_chain.As(&swap_chain_))) {
		return false;
	}

	frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

	factory->Release();

	D3D12_DESCRIPTOR_HEAP_DESC render_target_heap_desc = {};
	render_target_heap_desc.NumDescriptors = frame_cout_;
	render_target_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	render_target_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(d3d12device_->CreateDescriptorHeap(&render_target_heap_desc, IID_PPV_ARGS(&render_target_view_heap_)))) {
		return false;
	}

	render_target_descriptor_size_ = d3d12device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE render_target_handle(render_target_view_heap_->GetCPUDescriptorHandleForHeapStart());

	for (UINT index = 0; index < frame_cout_; ++index) {
		if (FAILED(swap_chain_->GetBuffer(index, IID_PPV_ARGS(&render_targets_[index])))) {
			return false;
		}
		d3d12device_->CreateRenderTargetView(render_targets_[index].Get(), nullptr, render_target_handle);
		render_target_handle.Offset(1, render_target_descriptor_size_);
	}

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(d3d12device_->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&depth_stencil_view_heap_)))) {
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	if (FAILED(d3d12device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			screen_width,
			screen_height,
			1, 0, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depth_stencil_resource_)
	))) {
		return false;
	}

	d3d12device_->CreateDepthStencilView(
		depth_stencil_resource_.Get(),
		&depthStencilDesc,
		depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart()
	);

	depth_stencil_descriptor_size_ = d3d12device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = screen_width;
	texDesc.Height = screen_height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE off_screen_color = {};
	off_screen_color.Color[0] = 0.0f;
	off_screen_color.Color[1] = 0.2f;
	off_screen_color.Color[2] = 0.4f;
	off_screen_color.Color[3] = 1.0f;
	off_screen_color.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (FAILED(d3d12device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&off_screen_color,
		IID_PPV_ARGS(&off_screen_texture_)
	))) {
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.NodeMask = 0;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	if (FAILED(d3d12device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&off_screen_srv_)))) {
		return false;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv_desc.Texture2D.MipLevels = 1;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
	d3d12device_->CreateShaderResourceView(
		off_screen_texture_.Get(),
		&srv_desc,
		off_screen_srv_->GetCPUDescriptorHandleForHeapStart()
	);

	D3D12_DESCRIPTOR_HEAP_DESC off_screen_heap_desc = {};
	off_screen_heap_desc.NumDescriptors = frame_cout_;
	off_screen_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	off_screen_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(d3d12device_->CreateDescriptorHeap(&off_screen_heap_desc, IID_PPV_ARGS(&off_screen_rtv_)))) {
		return false;
	}

	D3D12_RENDER_TARGET_VIEW_DESC off_screen_rtv_desc = {};
	off_screen_rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	off_screen_rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	off_screen_rtv_desc.Texture2D.MipSlice = 0;
	off_screen_rtv_desc.Texture2D.PlaneSlice = 0;
	d3d12device_->CreateRenderTargetView(
		off_screen_texture_.Get(), 
		&off_screen_rtv_desc, 
		off_screen_rtv_->GetCPUDescriptorHandleForHeapStart()
	);

	if (FAILED(d3d12device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&default_graphics_command_allocator_)))) {
		return false;
	}

	if (FAILED(d3d12device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&default_copy_command_allocator_)))) {
		return false;
	}

	if (FAILED(d3d12device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		default_graphics_command_allocator_.Get(),
		nullptr,
		IID_PPV_ARGS(&default_graphics_command_list_)))) {
		return false;
	}

	if (FAILED(d3d12device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		default_copy_command_allocator_.Get(),
		nullptr,
		IID_PPV_ARGS(&default_copy_command_list_)))) {
		return false;
	}

	if (FAILED(default_graphics_command_list_->Close())) {
		return false;
	}

	if (FAILED(default_copy_command_list_->Close())) {
		return false;
	}

	if (FAILED(d3d12device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)))) {
		return false;
	}

	fence_handle_ = CreateEvent(nullptr, false, false, nullptr);

	if (nullptr == fence_handle_) {
		if (FAILED((HRESULT_FROM_WIN32(GetLastError())))) {
			return false;
		}
	}

	D3D12_VIEWPORT view_port = {};
	view_port.Width = static_cast<float>(screen_width);
	view_port.Height = static_cast<float>(screen_height);
	view_port.MinDepth = 0.0f;
	view_port.MaxDepth = 1.0f;
	view_port.TopLeftX = 0.0f;
	view_port.TopLeftY = 0.0f;
	viewport_.push_back(view_port);

	D3D12_RECT rect = {};
	rect.right = static_cast<LONG>(screen_width);
	rect.bottom = static_cast<LONG>(screen_height);
	scissor_rect_.push_back(rect);

	float field_of_View, screen_aspect;

	field_of_View = (DirectX::XM_PI / 4.0f);
	screen_aspect = static_cast<float>(screen_width) / static_cast<float>(screen_height);

	projection_matrix_ = DirectX::XMMatrixIdentity();

	projection_matrix_ = DirectX::XMMatrixPerspectiveFovLH(field_of_View, screen_aspect, screen_near, screen_depth);

	world_matrix_ = DirectX::XMMatrixIdentity();

	ortho_matrix_ = DirectX::XMMatrixOrthographicLH(
		static_cast<float>(screen_width),
		static_cast<float>(screen_height),
		screen_near,
		screen_depth
	);

	return true;
}

bool DirectX12Device::ExecuteDefaultGraphicsCommandList(){

	if (FAILED(default_graphics_command_list_->Close())) {
		return false;
	}

	ID3D12CommandList *command_list[] = { default_graphics_command_list_.Get() };
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
	if (FAILED(default_graphics_command_list_->Reset(default_graphics_command_allocator_.Get(), nullptr))) {
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
	if (FAILED(default_graphics_command_allocator_->Reset())) {
		return false;
	}
	return true;
}

void DirectX12Device::SetGraphicsRootSignature(const RootSignaturePtr& graphics_rootsignature) {
	default_graphics_command_list_->SetGraphicsRootSignature(graphics_rootsignature.Get());
}

void DirectX12Device::SetPipelineStateObject(const PipelineStateObjectPtr& pso) {
	default_graphics_command_list_->SetPipelineState(pso.Get());
}

void DirectX12Device::SetDescriptorHeaps(UINT num_descriptors, ID3D12DescriptorHeap ** descriptor_arr) {
	default_graphics_command_list_->SetDescriptorHeaps(num_descriptors, descriptor_arr);
}

void DirectX12Device::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
	default_graphics_command_list_->SetGraphicsRootDescriptorTable(RootParameterIndex, BaseDescriptor);
}

void DirectX12Device::SetGraphicsRootConstantBufferView(UINT RootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	default_graphics_command_list_->SetGraphicsRootConstantBufferView(RootParameterIndex, BufferLocation);
}

void DirectX12Device::BindVertexBuffer(UINT start_slot, UINT num_views, const VertexBufferView* vertex_buffer) {
	default_graphics_command_list_->IASetVertexBuffers(start_slot, num_views, vertex_buffer);
}

void DirectX12Device::BindIndexBuffer(const IndexBufferView* index_buffer_view) {
	default_graphics_command_list_->IASetIndexBuffer(index_buffer_view);
}

void DirectX12Device::BeginDrawToOffScreen(){

	default_graphics_command_list_->RSSetViewports(1, &viewport_.at(0));
	default_graphics_command_list_->RSSetScissorRects(1, &scissor_rect_.at(0));

	default_graphics_command_list_->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			off_screen_texture_.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_RENDER_TARGET)
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
		off_screen_rtv_->GetCPUDescriptorHandleForHeapStart()
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(
		depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart()
	);

	default_graphics_command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

	const float clear_color[] = { 0.0f,0.2f,0.4f,1.0f };
	default_graphics_command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
	default_graphics_command_list_->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	default_graphics_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DirectX12Device::EndDrawToOffScreen(){

	default_graphics_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		off_screen_texture_.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_GENERIC_READ)
	);
}

void DirectX12Device::BeginPopulateGraphicsCommandList() {
	default_graphics_command_list_->RSSetViewports(1, &viewport_.at(0));
	default_graphics_command_list_->RSSetScissorRects(1, &scissor_rect_.at(0));

	default_graphics_command_list_->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			render_targets_[frame_index_].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET)
	);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(
		render_target_view_heap_->GetCPUDescriptorHandleForHeapStart(),
		frame_index_,
		render_target_descriptor_size_);

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv_handle(
		depth_stencil_view_heap_->GetCPUDescriptorHandleForHeapStart()
	);

	default_graphics_command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);

	const float clear_color[] = { 0.0f,0.2f,0.4f,1.0f };
	default_graphics_command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
	default_graphics_command_list_->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	default_graphics_command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DirectX12Device::EndPopulateGraphicsCommandList() {
	default_graphics_command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		render_targets_[frame_index_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT)
	);
}

void DirectX12Device::Draw(UINT IndexCountPerInstance, UINT InstanceCount,
	UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {

	default_graphics_command_list_->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount,
		StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

bool DirectX12Device::WaitForPreviousFrame() {

	const UINT64 fence = fence_value_;
	if (FAILED(default_graphics_command_queue_->Signal(fence_.Get(), fence_value_))) {
		return false;
	}
	++fence_value_;

	auto gpu_work_procress = fence_->GetCompletedValue();
	if (fence_->GetCompletedValue() < fence) {
		if (FAILED(fence_->SetEventOnCompletion(fence, fence_handle_))) {
			return false;
		}
		WaitForSingleObject(fence_handle_, INFINITE);
	}

	frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

	return true;
}

 void DirectX12Device::GetVideoCardInfo(char * card_name, int memory) {
	strcpy_s(card_name, 128, video_card_description_);
	memory = video_card_memory_;
}
