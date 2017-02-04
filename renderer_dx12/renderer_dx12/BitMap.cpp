#include "stdafx.h"
#include "BitMap.h"

bool Bitmap::Initialize(
	UINT screen_width, UINT screen_height,
	UINT bitmap_width, UINT bitmap_height
	){

	screen_width_ = screen_width;
	screen_height_ = screen_height;

	bitmap_width_ = bitmap_width;
	bitmap_height_ = bitmap_height;

	if(CHECK(InitializeBuffers())){
		return false;
	}

	if (CHECK(InitializeRootSignature())) {
		return false;
	}

	if (CHECK(InitializeGraphicsPipelineState())) {
		return false;
	}

	return true;
}

void Bitmap::SetVertexShader(const D3D12_SHADER_BYTECODE verte_shader_bitecode){
	vertex_shader_bitecode_ = verte_shader_bitecode;
}

void Bitmap::SetPixelShader(const D3D12_SHADER_BYTECODE pixel_shader_bitcode){
	pixel_shader_bitcode_ = pixel_shader_bitcode;
}

const D3D12_VERTEX_BUFFER_VIEW& Bitmap::GetVertexBufferView() const{
	return vertex_buffer_view_;
}

const D3D12_INDEX_BUFFER_VIEW& Bitmap::GetIndexBufferView() const{
	return index_buffer_view_;
}

const RootSignaturePtr& Bitmap::GetRootSignature() const{
	return root_signature_;
}

const PipelineStateObjectPtr& Bitmap::GetPipelineStateObject() const{
	return pso_depth_disabled_;
}

const ResourceSharedPtr& Bitmap::GetConstantBuffer() const{
	return constant_buffer_;
}

bool Bitmap::UpdateConstantBuffer(
	DirectX::XMMATRIX& world, 
	DirectX::XMMATRIX& view , 
	DirectX::XMMATRIX& orthogonality
	){
	
	D3D12_RANGE range;
	range.Begin = 0;
	range.End = 0;
	UINT8 *data_begin = 0;
	if (FAILED(constant_buffer_->Map(0, &range, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		matrix_constant_data_.world_ = world;
		matrix_constant_data_.view_ = view;
		matrix_constant_data_.orthogonality_ = orthogonality;
		memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
		constant_buffer_->Unmap(0, nullptr);
	}
	return true;
}

bool Bitmap::UpdateBitmapPos(int pos_x, int pos_y) {

	if (previous_pos_x_ == pos_x && previous_pos_y_ == pos_y) {
		return true;
	}

	previous_pos_x_ = pos_x;
	previous_pos_y_ = pos_y;

	auto left = (static_cast<float>(screen_width_) / 2) * -1 + static_cast<float>(pos_x);

	auto right = left + static_cast<float>(bitmap_width_);

	auto top = static_cast<float>(screen_height_) / 2 - static_cast<float>(pos_y);

	auto bottom = top - static_cast<float>(bitmap_height_);

	auto vertices = new VertexType[vertex_count_];
	if (!vertices){
		return false;
	}

	// First triangle.
	// Top left.
	vertices[0].position_ = DirectX::XMFLOAT3(left, top, 0.0f);  
	vertices[0].texture_ = DirectX::XMFLOAT2(0.0f, 0.0f);
	// Bottom right.
	vertices[1].position_ = DirectX::XMFLOAT3(right, bottom, 0.0f);
	vertices[1].texture_ = DirectX::XMFLOAT2(1.0f, 1.0f);
	// Bottom left.
	vertices[2].position_ = DirectX::XMFLOAT3(left, bottom, 0.0f);
	vertices[2].texture_ = DirectX::XMFLOAT2(0.0f, 1.0f);

	// Second triangle.
	// Top left.
	vertices[3].position_ = DirectX::XMFLOAT3(left, top, 0.0f);
	vertices[3].texture_ = DirectX::XMFLOAT2(0.0f, 0.0f);
	// Top right.
	vertices[4].position_ = DirectX::XMFLOAT3(right, top, 0.0f);
	vertices[4].texture_ = DirectX::XMFLOAT2(1.0f, 0.0f);
	// Bottom right.
	vertices[5].position_ = DirectX::XMFLOAT3(right, bottom, 0.0f);
	vertices[5].texture_ = DirectX::XMFLOAT2(1.0f, 1.0f);

	D3D12_RANGE range;
	range.Begin = 0;
	range.End = 0;
	UINT8 *data_begin = 0;
	if (FAILED(vertex_buffer_->Map(0, &range, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		memcpy(data_begin, vertices, sizeof(VertexType)*vertex_count_);
		vertex_buffer_->Unmap(0, nullptr);
	}

	delete[] vertices;
	vertices = nullptr;

	return true;
}

int Bitmap::GetIndexCount(){
	return index_count_;
}

bool Bitmap::InitializeBuffers()
{
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

	ZeroMemory(vertices, sizeof(VertexType)*vertex_count_);

	auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) * vertex_count_),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertex_buffer_)
		))) {
		return false;
	}

	UINT8 *vertex_data_begin = nullptr;
	CD3DX12_RANGE read_range(0, 0);
	if (FAILED(vertex_buffer_->Map(0, &read_range, reinterpret_cast<void**>(&vertex_data_begin)))) {
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
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t)*index_count_),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload_index_buffer)
		))) {
		return false;
	}

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t)*index_count_),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&index_buffer_)
		))) {
		return false;
	}

	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ID3D12CommandQueue *command_queue = nullptr;
	if (FAILED(device->CreateCommandQueue(
		&queue_desc,
		IID_PPV_ARGS(&command_queue)
		))) {
		return false;
	}

	ID3D12CommandAllocator *command_allocator = nullptr;
	if (FAILED(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&command_allocator)
		))) {
		return false;
	}

	ID3D12GraphicsCommandList *command_list = nullptr;
	if (FAILED(device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocator,
		nullptr,
		IID_PPV_ARGS(&command_list)
		))) {
		return false;
	}
	if (FAILED(command_list->Close())) {
		return false;
	}

	D3D12_SUBRESOURCE_DATA init_data = {};
	init_data.pData = indices;
	init_data.RowPitch = sizeof(uint16_t);
	init_data.SlicePitch = sizeof(uint16_t)*index_count_;

	if (FAILED(command_allocator->Reset())) {
		return false;
	}

	if (FAILED(command_list->Reset(command_allocator, nullptr))) {
		return false;
	}

	UpdateSubresources(command_list, index_buffer_.Get(), upload_index_buffer.Get(), 0, 0, 1, &init_data);

	command_list->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			index_buffer_.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_INDEX_BUFFER
			)
		);

	if (FAILED(command_list->Close())) {
		return false;
	}

	ID3D12Fence *fence = nullptr;
	if (FAILED(device->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)
		))) {
		return false;
	}

	auto fence_event = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == fence_event) {
		return false;
	}

	ID3D12CommandList* ppCommandLists[] = { command_list };
	command_queue->ExecuteCommandLists(1, ppCommandLists);

	if (FAILED(command_queue->Signal(fence, 2))) {
		return false;
	}

	if (fence->GetCompletedValue() < 2) {
		if (FAILED(fence->SetEventOnCompletion(1, fence_event))) {
			return false;
		}
		WaitForSingleObject(fence_event, INFINITE);
	}
	
	command_list->Release();
	command_allocator->Release();
	command_queue->Release();
	fence->Release();
	CloseHandle(fence_event);

	index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
	index_buffer_view_.SizeInBytes = sizeof(uint16_t)*index_count_;
	index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

	delete[] indices;
	indices = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
	cbv_heap_desc.NumDescriptors = 1;
	cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(MatrixBufferType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constant_buffer_)))) {
		return false;
	}

	ZeroMemory(&matrix_constant_data_, sizeof(MatrixBufferType));

	return true;
}

bool Bitmap::InitializeGraphicsPipelineState() {

	D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
	pso_desc.pRootSignature = root_signature_.Get();
	pso_desc.VS = vertex_shader_bitecode_;
	pso_desc.PS = pixel_shader_bitcode_;
	pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pso_desc.SampleMask = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets = 1;
	pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pso_desc.SampleDesc.Count = 1;

	auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();

	if (FAILED(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso_)))) {
		return false;
	}

	pso_desc.DepthStencilState.DepthEnable = false;
	if (FAILED(device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso_depth_disabled_)))) {
		return false;
	}

	return true;
}

bool Bitmap::InitializeRootSignature() {

	CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
	descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER root_parameters[2];
	root_parameters[0].InitAsDescriptorTable(1, &descriptor_ranges_srv[0], D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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
		ARRAYSIZE(root_parameters), 
		root_parameters, 
		1, 
		&sampler_desc,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		);

	ID3DBlob* signature_blob = nullptr;
	ID3DBlob *signature_error = nullptr;
	if (FAILED(D3D12SerializeRootSignature(
		&rootsignature_Layout,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature_blob,
		&signature_error
		))) {
		return false;
	}

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateRootSignature(
		0,
		signature_blob->GetBufferPointer(),
		signature_blob->GetBufferSize(),
		IID_PPV_ARGS(&root_signature_)
		))) {
		return false;
	}

	return true;
}
