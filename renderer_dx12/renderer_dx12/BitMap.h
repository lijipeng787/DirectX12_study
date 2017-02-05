#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <memory>
#include <DirectXMath.h>

#include "TypeDefine.h"
#include "Texture.h"
#include "Material.h"

class Bitmap :public Effect::Material{
private:
	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_;
	};

	struct MatrixBufferType {
		DirectX::XMFLOAT4X4 world_;
		DirectX::XMFLOAT4X4 view_;
		DirectX::XMFLOAT4X4 orthogonality_;
	};
public:
	Bitmap() {}

	Bitmap(const Bitmap& rhs) = delete;

	Bitmap& operator=(const Bitmap& rhs) = delete;

	virtual ~Bitmap() {}
public:
	bool Initialize(
		UINT screen_width, UINT screen_height,
		UINT bitmap_width, UINT bitmap_height
	);

	void SetVertexShader(const VertexShaderByteCode& verte_shader_bitecode);

	void SetPixelShader(const PixelShaderByteCode& pixel_shader_bitcode);

	VertexBufferView GetVertexBufferView()const;

	VertexBufferView GetIndexBufferView()const;

	RootSignaturePtr GetRootSignature()const;

	PipelineStateObjectPtr GetPipelineStateObject()const;

	ResourceSharedPtr GetConstantBuffer()const;

	bool UpdateConstantBuffer(DirectX::XMMATRIX& world, DirectX::XMMATRIX& view, DirectX::XMMATRIX& orthogonality);

	bool UpdateBitmapPosition(int pos_x, int pos_y);

	const int GetIndexCount();
private:
	bool InitializeBuffers();

	bool InitializeGraphicsPipelineState();

	bool InitializeRootSignature();
private:
	ResourceSharedPtr vertex_buffer_ = nullptr;

	VertexBufferView vertex_buffer_view_;

	ResourceSharedPtr index_buffer_ = nullptr;

	IndexBufferView index_buffer_view_;

	ResourceSharedPtr constant_buffer_ = nullptr;

	MatrixBufferType matrix_constant_data_ = {};

	UINT index_count_ = 0, vertex_count_ = 0;

	UINT screen_width_ = 0, screen_height_ = 0;

	UINT bitmap_height_ = 0, bitmap_width_ = 0;

	int previous_pos_x_ = -1, previous_pos_y_ = -1;
};

#endif // !_BITMAP_H_