#include "stdafx.h"
#include "modelclass.h"

using namespace std;
using namespace DirectX;

bool Model::Initialize(WCHAR* model_filename, WCHAR **texture_filename_arr) {

	if (CHECK(LoadModel(model_filename))) {
		return false;
	}
	if (CHECK(InitializeBuffers())) {
		return false;
	}

	texture_ = std::make_shared<Texture>();
	texture_->set_texture_array_capacity(3);
	if (CHECK(LoadTexture(texture_filename_arr))) {
		return false;
	}

	return true;
}

bool Model::LoadModel(WCHAR* filename) {

	std::ifstream fin;
	fin.open(filename);
	if (fin.fail()) {
		return false;
	}

	char input = ' ';
	fin.get(input);
	while (input != ':') {
		fin.get(input);
	}

	fin >> vertex_count_;
	index_count_ = vertex_count_;

	tem_model_ = new ModelType[vertex_count_];
	if (!tem_model_) {
		return false;
	}

	fin.get(input);
	while (input != ':') {
		fin.get(input);
	}
	fin.get(input);
	fin.get(input);

	for (UINT i = 0; i < vertex_count_; ++i) {
		fin >> tem_model_[i].x_ >> tem_model_[i].y_ >> tem_model_[i].z_;
		fin >> tem_model_[i].tu_ >> tem_model_[i].tv_;
		fin >> tem_model_[i].nx_ >> tem_model_[i].ny_ >> tem_model_[i].nz_;
	}

	fin.close();
	return true;
}

bool Model::InitializeBuffers() {
	
	auto vertices = new VertexType[vertex_count_];
	if (!vertices) {
		return false;
	}

	auto indices = new uint16_t[index_count_];
	if (!indices) {
		return false;
	}

	for (UINT i = 0; i < vertex_count_; ++i) {
		vertices[i].position_ = DirectX::XMFLOAT3(tem_model_[i].x_, tem_model_[i].y_, tem_model_[i].z_);
		vertices[i].texture_ = DirectX::XMFLOAT2(tem_model_[i].tu_, tem_model_[i].tv_);
		vertices[i].normal_ = DirectX::XMFLOAT3(tem_model_[i].nx_, tem_model_[i].ny_, tem_model_[i].nz_);
		indices[i] = i;
	}

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

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(uint16_t) * index_count_),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&index_buffer_)
		))) {
		return false;
	}

	UINT8 *index_data_begin = nullptr;
	if (FAILED(index_buffer_->Map(0, &read_range, reinterpret_cast<void**>(&index_data_begin)))) {
		return false;
	}
	memcpy(index_data_begin, indices, sizeof(uint16_t) * index_count_);
	index_buffer_->Unmap(0, nullptr);

	index_buffer_view_.BufferLocation = index_buffer_->GetGPUVirtualAddress();
	index_buffer_view_.SizeInBytes = sizeof(uint16_t) * index_count_;
	index_buffer_view_.Format = DXGI_FORMAT_R16_UINT;

	delete[] vertices;
	vertices = nullptr;

	delete[] indices;
	indices = nullptr;
	
	delete[] tem_model_;
	tem_model_ = nullptr;

	return true;
}

ModelMaterial::ModelMaterial(){
}

ModelMaterial::~ModelMaterial(){
}

bool ModelMaterial::Initialize(){

	auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(MatrixBufferType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&matrix_constant_buffer_)))) {
		return false;
	}

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&light_constant_buffer_)))) {
		return false;
	}

	if (FAILED(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(FogBufferType)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&fog_constant_buffer_)))) {
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

DescriptorHeapPtr ModelMaterial::GetShaderRescourceView() const{
}

ResourceSharedPtr ModelMaterial::GetMatrixConstantBuffer() const{
	return matrix_constant_buffer_;
}

ResourceSharedPtr ModelMaterial::GetLightConstantBuffer() const{
	return light_constant_buffer_;
}

ResourceSharedPtr ModelMaterial::GetFogConstantBuffer() const{
	return fog_constant_buffer_;
}

bool ModelMaterial::InitializeRootSignature() {

	CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
	descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER root_parameters[4];
	root_parameters[0].InitAsDescriptorTable(1, descriptor_ranges_srv, D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[1].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	root_parameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	root_parameters[3].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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

	RootSignaturePtr root_signature = {};
	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateRootSignature(
		0,
		signature_blob->GetBufferPointer(),
		signature_blob->GetBufferSize(),
		IID_PPV_ARGS(&root_signature)
	))) {
		return false;
	}
	SetRootSignature(root_signature);

	return true;
}

bool ModelMaterial::InitializeGraphicsPipelineState() {

	D3D12_INPUT_ELEMENT_DESC input_element_descs[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.InputLayout = { input_element_descs, _countof(input_element_descs) };
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

	PipelineStateObjectPtr pso = {};
	if (FAILED(DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device()->CreateGraphicsPipelineState(
		&pso_desc,
		IID_PPV_ARGS(&pso)
	))) {
		return false;
	}
	SetPSOByName("model_normal", pso);

	return true;
}

bool ModelMaterial::UpdateMatrixConstant(const XMMATRIX & world, const XMMATRIX & view, const XMMATRIX & projection) {

	UINT8 *data_begin = 0;
	if (FAILED(matrix_constant_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		XMStoreFloat4x4(&matrix_constant_data_.world_, world);
		XMStoreFloat4x4(&matrix_constant_data_.view_, view);
		XMStoreFloat4x4(&matrix_constant_data_.projection_, projection);
		memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
		matrix_constant_buffer_->Unmap(0, nullptr);
	}

	return true;
}

bool ModelMaterial::UpdateLightConstant(const XMFLOAT4& ambient_color, const XMFLOAT4& diffuse_color, const XMFLOAT3& direction) {

	UINT8 *data_begin = 0;
	if (FAILED(light_constant_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		light_constant_data_.ambient_color_ = ambient_color;
		light_constant_data_.diffuse_color_ = diffuse_color;
		light_constant_data_.direction_ = direction;
		light_constant_data_.padding_ = 0.0f;
		memcpy(data_begin, &light_constant_data_, sizeof(LightType));
		light_constant_buffer_->Unmap(0, nullptr);
	}

	return true;
}

bool ModelMaterial::UpdateFogConstant(float fog_begin, float fog_end) {

	UINT8 *data_begin = 0;
	if (FAILED(fog_constant_buffer_->Map(0, nullptr, reinterpret_cast<void**>(&data_begin)))) {
		return false;
	}
	else {
		fog_constant_data_.fog_start_ = fog_begin;
		fog_constant_data_.fog_end_ = fog_end;
		memcpy(data_begin, &fog_constant_data_, sizeof(FogBufferType));
		fog_constant_buffer_->Unmap(0, nullptr);
	}

	return true;
}