#ifndef _TEXTCLASS_H_
#define _TEXTCLASS_H_

#include "fontclass.h"
#include "fontshaderclass.h"

class Text {
private:
	struct ConstantBufferType {
		DirectX::XMMATRIX world_;
		DirectX::XMMATRIX base_view_;
		DirectX::XMMATRIX orthonality_;
	};

	struct PixelBufferType {
		DirectX::XMFLOAT4 pixel_color_;
	};

	struct SentenceType {
		ResourceSharedPtr vertex_buffer_ = nullptr;
		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};
		ResourceSharedPtr index_buffer_ = nullptr;
		D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

		int vertex_count_ = 0;
		int index_count_ = 0;
		int max_length_ = 0;
		float red_ = 0.0f, green_ = 0.0f, blue_ = 0.0f;
	};

	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_;
	};
public:
	Text() {}

	Text(const Text& copy) = delete;
    
    Text& operator=(const Text& rhs) = delete;

	~Text() {/* todo deallocate sentence vector memory */ }
public:
	bool SetFps(int fps);
	
	bool SetCpu(int cpu_percentage_value);
public:
	bool Initialize(int screenWidth, int screenHeight, const DirectX::XMMATRIX& baseViewMatrix);

	bool LoadFont(WCHAR *font_data, WCHAR **font_texture);

	void SetVertexShader(const D3D12_SHADER_BYTECODE verte_shader_bitecode) {
		vertex_shader_bitecode_ = verte_shader_bitecode;
	}

	void SetPixelShader(const D3D12_SHADER_BYTECODE pixel_shader_bitcode) {
		pixel_shader_bitecode_ = pixel_shader_bitcode;
	}

	bool InitializeRootSignature();

	bool InitializeGraphicsPipelineState();

	bool InitializeConstantBuffer();

	bool InitializeSentence(SentenceType** sentence, int max_length);

	const UINT GetIndexCount(int index)const {
		return sentence_vector_.at(index)->index_count_;
	}

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView(int index)const {
		return sentence_vector_.at(index)->vertex_buffer_view_;
	}

	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView(int index)const {
		return sentence_vector_.at(index)->index_buffer_view_;
	}

	const int GetSentenceCount()const { return static_cast<int>(sentence_vector_.size()); }

	const RootSignaturePtr& GetRootSignature()const { return root_signature_; }

	const PipelineStateObjectPtr& GetNormalPso()const { return pipeline_state_object_; }

	const PipelineStateObjectPtr& GetBlendEnabledPso()const { return blend_enabled_pso_; }

	const DescriptorHeapPtr& GetShaderRescourceView()const { return font_->GetTexture(); }

	const ResourceSharedPtr& GetMatrixConstantBuffer()const { return matrix_constant_buffer_; }

	const ResourceSharedPtr& GetPixelConstantBuffer()const { return pixel_color_constant_buffer_; }

	bool UpdateSentenceVertexBuffer(SentenceType* sentence, WCHAR* text,
		int positionX, int positionY, float red, float green, float blue
	);

	bool UpdateMatrixConstant(
		DirectX::XMMATRIX& world,
		DirectX::XMMATRIX& base_view,
		DirectX::XMMATRIX& orthonality
		);

	bool UpdateLightConstant(
		DirectX::XMFLOAT4& pixel_color
		);
private:
	ResourceSharedPtr matrix_constant_buffer_ = nullptr;

	ConstantBufferType matrix_constant_data_ = {};

	ResourceSharedPtr pixel_color_constant_buffer_ = nullptr;

	PixelBufferType pixel_color_constant_data_ = {};

	RootSignaturePtr root_signature_ = nullptr;

	PipelineStateObjectPtr pipeline_state_object_ = nullptr;

	PipelineStateObjectPtr blend_enabled_pso_ = nullptr;

	D3D12_SHADER_BYTECODE vertex_shader_bitecode_ = {};

	D3D12_SHADER_BYTECODE pixel_shader_bitecode_ = {};
private:
	std::shared_ptr<Font> font_ = nullptr;

	int screen_width_ = 0, screen_height_ = 0;

	DirectX::XMMATRIX base_view_matrix_ = {};

	std::vector<SentenceType*> sentence_vector_;
};

#endif