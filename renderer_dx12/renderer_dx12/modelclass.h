#ifndef _MODELCLASS_H_
#define _MODELCLASS_H_

#include <DirectXMath.h>
#include <fstream>
#include <memory>

#include "../d3d12 common/DirectX12Device.h"
#include "../d3d12 common/Texture.h"

class Model {
private:
	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_;
		DirectX::XMFLOAT3 normal_;
	};

	struct ModelType {
		float x_, y_, z_;
		float tu_, tv_;
		float nx_, ny_, nz_;
	};

	struct MatrixBufferType {
		DirectX::XMMATRIX world_;
		DirectX::XMMATRIX view_;
		DirectX::XMMATRIX projection_;
		DirectX::XMMATRIX padding_;
	};

	struct LightType {
		DirectX::XMFLOAT4 ambient_color_;
		DirectX::XMFLOAT4 diffuse_color_;
		DirectX::XMFLOAT3 direction_;
		float padding_;
	};

	struct FogBufferType {
		float fog_start_;
		float fog_end_;
		float padding1_;
		float padding2_;
	};
public:
	Model() {}

	Model(const Model& rhs) = delete;

	Model& operator=(const Model& rhs) = delete;

	~Model() {}
public:
	bool Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr);

	bool Model::LoadTexture(WCHAR **texture_filename_arr) { 
		return texture_->set_texture(texture_filename_arr); 
	}

	void Model::SetVertexShader(const D3D12_SHADER_BYTECODE & verte_shader_bitecode) { 
		vertex_shader_bitecode_ = verte_shader_bitecode; 
	}

	void Model::SetPixelShader(const D3D12_SHADER_BYTECODE & pixel_shader_bitcode) { 
		pixel_shader_bitecode_ = pixel_shader_bitcode;
	}

	UINT Model::GetIndexCount() const { return index_count_; }

	const D3D12_VERTEX_BUFFER_VIEW& Model::GetVertexBufferView() const { return vertex_buffer_view_; }

	const D3D12_INDEX_BUFFER_VIEW& Model::GetIndexBufferView() const { return index_buffer_view_; }

	const RootSignaturePtr& GetRootSignature() const { return root_signature_; }

	const PipelineStateObjectPtr& GetPipelineStateObject() const { return pipeline_state_object_; }

	const DescriptorHeapPtr& GetShaderRescourceView() const { return texture_->get_textures_descriptor_heap(); }

	const ResourceSharedPtr& GetMatrixConstantBuffer() const { return matrix_constant_buffer_; }

	const ResourceSharedPtr& GetLightConstantBuffer() const { return light_constant_buffer_; }

	const ResourceSharedPtr& GetFogConstantBuffer()const { return fog_constant_buffer_; }

	bool UpdateMatrixConstant(
		DirectX::XMMATRIX& world,
		DirectX::XMMATRIX& view,
		DirectX::XMMATRIX& projection
		);

	bool UpdateLightConstant(
		DirectX::XMFLOAT4& ambient_color,
		DirectX::XMFLOAT4& diffuse_color,
		DirectX::XMFLOAT3& direction
		);

	bool UpdateFogConstant(float fog_begin, float fog_end);
private:
	bool LoadModel(WCHAR *filename);

	bool InitializeBuffers();

	bool InitializeRootSignature();

	bool InitializeGraphicsPipelineState();

	bool LoadTexture(WCHAR *texture_filename);
private:
	ResourceSharedPtr vertex_buffer_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

	ResourceSharedPtr index_buffer_ = nullptr;

	D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

	ResourceSharedPtr matrix_constant_buffer_ = nullptr;

	MatrixBufferType matrix_constant_data_ = {};

	ResourceSharedPtr light_constant_buffer_ = nullptr;

	LightType light_constant_data_ = {};

	ResourceSharedPtr fog_constant_buffer_ = nullptr;

	FogBufferType fog_constant_data_ = {};

	RootSignaturePtr root_signature_ = nullptr;

	PipelineStateObjectPtr pipeline_state_object_ = nullptr;

	D3D12_SHADER_BYTECODE vertex_shader_bitecode_ = {};

	D3D12_SHADER_BYTECODE pixel_shader_bitecode_ = {};
private:
	UINT vertex_count_ = 0;

	UINT index_count_ = 0;

	ModelType *tem_model_ = nullptr;

	std::shared_ptr<Texture> texture_ = nullptr;
};

#endif // !_MODELCLASS_H_