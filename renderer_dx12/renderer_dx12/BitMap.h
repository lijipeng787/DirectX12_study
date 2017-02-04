#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <memory>
#include <DirectXMath.h>
#include "../d3d12 common/Texture.h"

class Bitmap {
private:
	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_;
	};

	struct MatrixBufferType {
		DirectX::XMMATRIX world_;
		DirectX::XMMATRIX view_;
		DirectX::XMMATRIX orthogonality_;
		DirectX::XMMATRIX padding_;
	};
public:
	Bitmap() {}

	Bitmap(const Bitmap& rhs) = delete;

	Bitmap& operator=(const Bitmap& rhs) = delete;

	~Bitmap() {}
public:
	bool Initialize(
		UINT screen_width, UINT screen_height,
		UINT bitmap_width, UINT bitmap_height
	);

	void SetVertexShader(const D3D12_SHADER_BYTECODE verte_shader_bitecode);

	void SetPixelShader(const D3D12_SHADER_BYTECODE pixel_shader_bitcode);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView()const;

	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView()const;

	const RootSignaturePtr& GetRootSignature()const;

	const PipelineStateObjectPtr& GetPipelineStateObject()const;

	const ResourceSharedPtr& GetConstantBuffer()const;

	bool UpdateConstantBuffer(
		DirectX::XMMATRIX& world,
		DirectX::XMMATRIX& view,
		DirectX::XMMATRIX& orthogonality
	);

	bool UpdateBitmapPos(int pos_x, int pos_y);

	int GetIndexCount();
private:
	bool InitializeBuffers();

	bool InitializeGraphicsPipelineState();

	bool InitializeRootSignature();
private:
	D3D12_SHADER_BYTECODE vertex_shader_bitecode_ = {};

	D3D12_SHADER_BYTECODE pixel_shader_bitcode_ = {};

	RootSignaturePtr root_signature_ = nullptr;

	PipelineStateObjectPtr pso_ = nullptr;

	PipelineStateObjectPtr pso_depth_disabled_ = nullptr;

	ResourceSharedPtr vertex_buffer_ = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;

	ResourceSharedPtr index_buffer_ = nullptr;

	D3D12_INDEX_BUFFER_VIEW index_buffer_view_;

	ResourceSharedPtr constant_buffer_ = nullptr;

	MatrixBufferType matrix_constant_data_ = {};

	UINT index_count_ = 0;

	UINT vertex_count_ = 0;

	UINT screen_width_ = 0;

	UINT screen_height_ = 0;

	UINT bitmap_height_ = 0;

	UINT bitmap_width_ = 0;

	int previous_pos_x_ = -1;

	int previous_pos_y_ = -1;
};

#endif // !_BITMAP_H_