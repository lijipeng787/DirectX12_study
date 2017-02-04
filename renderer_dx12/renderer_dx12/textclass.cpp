#include "stdafx.h"
#include "textclass.h"

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
	if (CHECK(UpdateSentenceVertexBuffer(sentence_vector_.at(1), fpsString, 20, 20, red, green, blue))) {
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

	if (CHECK(UpdateSentenceVertexBuffer(sentence_vector_.at(0), cpuString, 20, 40, 0.0f, 1.0f, 0.0f))) {
		return false;
	}

	return true;
}

bool Text::Initialize(int screen_width, int screen_height, const DirectX::XMMATRIX& base_view_matrix) {

	// Store the screen width and height.
	screen_width_ = screen_width;
	screen_height_ = screen_height;

	// Store the base view matrix.
	base_view_matrix_ = base_view_matrix;

	SentenceType *sentence1 = nullptr;
	// Initialize the first sentence.
	if (CHECK(InitializeSentence(&sentence1, 16))) {
		return false;
	}

	// Now update the sentence vertex buffer with the new string information.
	if (CHECK(UpdateSentenceVertexBuffer(sentence1, L"Hello", 100, 100, 1.0f, 1.0f, 1.0f))) {
		return false;
	}

	sentence_vector_.push_back(sentence1);

	SentenceType *sentence2 = nullptr;
	// Initialize the first sentence.
	if (CHECK(InitializeSentence(&sentence2, 16))) {
		return false;
	}

	// Now update the sentence vertex buffer with the new string information.
	if (CHECK(UpdateSentenceVertexBuffer(sentence2, L"World", 100, 200, 1.0f, 1.0f, 0.0f))) {
		return false;
	}
	
	sentence_vector_.push_back(sentence2);
	
	return true;
}

bool Text::LoadFont(WCHAR *font_data, WCHAR **font_texture) {

	font_ = std::make_shared<Font>();
	if (!font_) {
		return false;
	}

	if (CHECK(font_->Initialize(font_data, font_texture))) {
		return false;
	}
	return true;
}

bool Text::InitializeRootSignature(){

	CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
	descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER root_parameters[3];
	root_parameters[0].InitAsDescriptorTable(1, &descriptor_ranges_srv[0], D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	root_parameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

	CD3DX12_ROOT_SIGNATURE_DESC rootsignature_layout(
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
		&rootsignature_layout,
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

bool Text::InitializeGraphicsPipelineState(){

	D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
	pso_desc.pRootSignature = root_signature_.Get();
	pso_desc.VS = vertex_shader_bitecode_;
	pso_desc.PS = pixel_shader_bitecode_;
	pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pso_desc.SampleMask = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets = 1;
	pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pso_desc.SampleDesc.Count = 1;

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateGraphicsPipelineState(
		&pso_desc,
		IID_PPV_ARGS(&pipeline_state_object_)
		))) {
		return false;
	}

	D3D12_BLEND_DESC blend_desc = {};
	blend_desc.AlphaToCoverageEnable = false;
	blend_desc.IndependentBlendEnable = false;
	blend_desc.RenderTarget[0].BlendEnable = true;
	blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blend_desc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

	pso_desc.BlendState = blend_desc;

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateGraphicsPipelineState(
		&pso_desc,
		IID_PPV_ARGS(&blend_enabled_pso_)
		))) {
		return false;
	}

	return true;
}

bool Text::InitializeConstantBuffer(){

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBufferType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&matrix_constant_buffer_)))) {
		return false;
	}

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(PixelBufferType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&pixel_color_constant_buffer_)))) {
		return false;
	}

	return true;
}

bool Text::InitializeSentence(SentenceType** sentence, int max_length){

	// Create a new sentence object.
	*sentence = new SentenceType;
	if(!*sentence){
		return false;
	}

	// Set the maximum length of the sentence.
	(*sentence)->max_length_ = max_length;

	// Set the number of vertices in the vertex array.
	(*sentence)->vertex_count_ = 6 * max_length;

	// Set the number of indexes in the index array.
	(*sentence)->index_count_ = (*sentence)->vertex_count_;

	// Create the index array.
	auto indices = new uint16_t[(*sentence)->index_count_];
	if(!indices){
		return false;
	}

	// Initialize the index array.
	for(auto i=0; i<(*sentence)->index_count_; ++i){
		indices[i] = i;
	}

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) * (*sentence)->vertex_count_),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&(*sentence)->vertex_buffer_)
		))) {
		return false;
	}

	(*sentence)->vertex_buffer_view_.BufferLocation = (*sentence)->vertex_buffer_->GetGPUVirtualAddress();
	(*sentence)->vertex_buffer_view_.SizeInBytes = sizeof(VertexType) * (*sentence)->vertex_count_;
	(*sentence)->vertex_buffer_view_.StrideInBytes = sizeof(VertexType);

    auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();
    
    ResourceSharedPtr upload_index_buffer = nullptr;
	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * (*sentence)->index_count_),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload_index_buffer)
		))) {
		return false;
	}

	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * (*sentence)->index_count_),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&(*sentence)->index_buffer_)
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
	init_data.SlicePitch = sizeof(uint16_t) * (*sentence)->index_count_;
    
    if (FAILED(command_allocator->Reset())) {
		return false;
	}

	if (FAILED(command_list->Reset(command_allocator, nullptr))) {
		return false;
	}
    
    UpdateSubresources(command_list, (*sentence)->index_buffer_.Get(), upload_index_buffer.Get(), 0, 0, 1, &init_data);

	command_list->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			(*sentence)->index_buffer_.Get(),
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

	if (FAILED(command_queue->Signal(fence, 1))) {
		return false;
	}

	if (fence->GetCompletedValue() < 1) {
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

	(*sentence)->index_buffer_view_.BufferLocation = (*sentence)->index_buffer_->GetGPUVirtualAddress();
	(*sentence)->index_buffer_view_.SizeInBytes = sizeof(uint16_t) * (*sentence)->index_count_;
	(*sentence)->index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

	// Release the index array as it is no longer needed.
	delete[] indices;
	indices = nullptr;

	return true;
}

bool Text::UpdateSentenceVertexBuffer(
	SentenceType* sentence,
	WCHAR *text,
	int positionX, int positionY,
	float red, float green, float blue
	) {

	// Store the color of the sentence.
	sentence->red_ = red;
	sentence->green_ = green;
	sentence->blue_ = blue;

	// Get the number of letters in the sentence.
	auto numLetters = (int)wcslen(text);

	// Check for possible buffer overflow.
	if (numLetters > sentence->max_length_) {
		return false;
	}

	// Create the vertex array.
	auto vertices = new VertexType[sentence->vertex_count_];
	if (!vertices) {
		return false;
	}

	// Initialize vertex array to zeros at first.
	memset(vertices, 0, (sizeof(VertexType) * sentence->vertex_count_));

	// Calculate the X and Y pixel position on the screen to start drawing to.
	auto drawX = static_cast<float>(((screen_width_ / 2) * -1) + positionX);
	auto drawY = static_cast<float>((screen_height_ / 2) - positionY);

	// Use the font class to build the vertex array from the sentence text and sentence draw location.
	font_->BuildVertexArray((void*)vertices, text, drawX, drawY);

    auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();

	ResourceSharedPtr upload_vertex_buffer = nullptr;
	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(VertexType) * sentence->vertex_count_),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&upload_vertex_buffer)
		))) {
		return false;
	}
    
	D3D12_SUBRESOURCE_DATA init_data = {};
	init_data.pData = vertices;
	init_data.RowPitch = sizeof(VertexType);
	init_data.SlicePitch = sizeof(VertexType) * sentence->vertex_count_;

	auto command_allocator = DirectX12Device::GetD3d12DeviceInstance()->GetDefaultGraphicsCommandAllocator().Get();
	auto command_list = DirectX12Device::GetD3d12DeviceInstance()->GetDefaultGraphicsCommandList().Get();

	if (FAILED(command_allocator->Reset())) {
		return false;
	}
	if (FAILED(command_list->Reset(command_allocator, nullptr))) {
		return false;
	}

	UpdateSubresources(command_list, sentence->vertex_buffer_.Get(), upload_vertex_buffer.Get(), 0, 0, 1, &init_data);

	command_list->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			sentence->vertex_buffer_.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
		)
	);

	if (FAILED(command_list->Close())) {
		return false;
	}

	auto command_queue = DirectX12Device::GetD3d12DeviceInstance()->GetDefaultGraphicsCommandQueeue().Get();
	ID3D12CommandList* ppCommandLists[] = { command_list };
	command_queue->ExecuteCommandLists(1, ppCommandLists);

	auto fence_event = CreateEvent(nullptr, false, false, nullptr);
	if (nullptr == fence_event) {
		return false;
	}

	ID3D12Fence *fence = nullptr;
	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateFence(
		0,
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&fence)
	))) {
		return false;
	}

	if (FAILED(command_queue->Signal(fence, 1))) {
		return false;
	}

	if (fence->GetCompletedValue() < 1) {
		if (FAILED(fence->SetEventOnCompletion(1, fence_event))) {
			return false;
		}
		WaitForSingleObject(fence_event, INFINITE);
	}

	fence->Release();
	CloseHandle(fence_event);

	// Release the vertex array as it is no longer needed.
	delete[] vertices;
	vertices = nullptr;

	return true;
}

bool Text::UpdateMatrixConstant(
	DirectX::XMMATRIX & world, 
	DirectX::XMMATRIX & base_view, 
	DirectX::XMMATRIX & orthonality){

	D3D12_RANGE range;
	range.Begin = 0;
	range.End = 0;
	UINT8 *data_begin = 0;
	if (FAILED(matrix_constant_buffer_->Map(0, &range, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		matrix_constant_data_.world_ = world;
		matrix_constant_data_.base_view_ = base_view;
		matrix_constant_data_.orthonality_ = orthonality;
		memcpy(data_begin, &matrix_constant_data_, sizeof(ConstantBufferType));
		matrix_constant_buffer_->Unmap(0, nullptr);
	}

	return true;
}

bool Text::UpdateLightConstant(DirectX::XMFLOAT4 & pixel_color){

	D3D12_RANGE range;
	range.Begin = 0;
	range.End = 0;
	UINT8 *data_begin = 0;
	if (FAILED(pixel_color_constant_buffer_->Map(0, &range, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		pixel_color_constant_data_.pixel_color_ = pixel_color;
		memcpy(data_begin, &pixel_color_constant_data_, sizeof(PixelBufferType));
		pixel_color_constant_buffer_->Unmap(0, nullptr);
	}

	return true;
}
