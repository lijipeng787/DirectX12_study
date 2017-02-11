#ifndef _TEXTCLASS_H_
#define _TEXTCLASS_H_

#include "Material.h"
#include "fontclass.h"

class Text : public Effect::Material{
public:
	Text() {}

	Text(const Text& copy) = delete;

	Text& operator=(const Text& rhs) = delete;

	virtual ~Text() {/* todo deallocate sentence vector memory */ }
private:
	struct ConstantBufferType {
		DirectX::XMFLOAT4X4 world_;
		DirectX::XMFLOAT4X4 base_view_;
		DirectX::XMFLOAT4X4 orthonality_;
	};

	struct PixelBufferType {
		DirectX::XMFLOAT4 pixel_color_;
	};

	struct SentenceType {
		ResourceSharedPtr vertex_buffer_ = nullptr;

		D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

		ResourceSharedPtr index_buffer_ = nullptr;

		D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

		int vertex_count_ = 0, index_count_ = 0, max_length_ = 0;
		float red_ = 0.0f, green_ = 0.0f, blue_ = 0.0f;
	};

	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_;
	};
public:
	bool SetFps(int fps);
	
	bool SetCpu(int cpu_percentage_value);
public:
	bool Initialize(int screenWidth, int screenHeight, const DirectX::XMMATRIX& baseViewMatrix);

	bool LoadFont(WCHAR *font_data, WCHAR **font_texture);

	bool InitializeRootSignature();

	bool InitializeGraphicsPipelineState();

	bool InitializeConstantBuffer();

	bool InitializeSentence(SentenceType** sentence, int max_length);

	UINT GetIndexCount(int index)const {
		return sentence_vector_.at(index)->index_count_;
	}

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView(int index)const {
		return sentence_vector_.at(index)->vertex_buffer_view_;
	}

	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView(int index)const {
		return sentence_vector_.at(index)->index_buffer_view_;
	}

	int GetSentenceCount()const { return static_cast<int>(sentence_vector_.size()); }

	DescriptorHeapPtr GetShaderRescourceView()const { return font_->GetTexture(); }

	ResourceSharedPtr GetMatrixConstantBuffer()const { return matrix_constant_buffer_; }

	ResourceSharedPtr GetPixelConstantBuffer()const { return pixel_color_constant_buffer_; }

	bool UpdateSentenceVertexBuffer(
		SentenceType* sentence, WCHAR* text,
		int positionX, int positionY, 
		float red, float green, float blue
	);

	bool UpdateMatrixConstant(
		const DirectX::XMMATRIX& world,
		const DirectX::XMMATRIX& base_view,
		const DirectX::XMMATRIX& orthonality
		);

	bool UpdateLightConstant(
		const DirectX::XMFLOAT4& pixel_color
		);
private:
	ResourceSharedPtr matrix_constant_buffer_ = nullptr;

	ConstantBufferType matrix_constant_data_ = {};

	ResourceSharedPtr pixel_color_constant_buffer_ = nullptr;

	PixelBufferType pixel_color_constant_data_ = {};
private:
	std::shared_ptr<Font> font_ = nullptr;

	int screen_width_ = 0, screen_height_ = 0;

	DirectX::XMMATRIX base_view_matrix_ = {};

	std::vector<SentenceType*> sentence_vector_;
};

#endif