#ifndef _FONTCLASS_H_
#define _FONTCLASS_H_

#include <DirectXMath.h>
#include <fstream>
#include <memory>

#include "../d3d12 common/Texture.h"

class Font{
private:
	struct FontType{
		float left_;
		float right_;
		int size_;
	};

	struct VertexType{
		DirectX::XMFLOAT3 position_;
	    DirectX::XMFLOAT2 texture_;
	};
public:
	Font() {}

	Font(const Font& rhs) = delete;

	Font& operator==(const Font& rhs) = delete;
	
	~Font() { if (nullptr != font_) { delete font_; } }
public:
	bool Initialize(WCHAR* font_filename, WCHAR **texture_filename);

	const DescriptorHeapPtr& GetTexture()const { return texture_->get_textures_descriptor_heap(); }

	void BuildVertexArray(void* vertices, WCHAR* sentence, float drawX, float drawY);
private:
	bool LoadFontData(WCHAR* font_name);
	
	bool LoadTexture(WCHAR **texture_name_arr);
private:
	FontType *font_ = nullptr;

	std::shared_ptr<Texture> texture_ = nullptr;
};

#endif