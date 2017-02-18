#ifndef _FONTCLASS_H_
#define _FONTCLASS_H_

#include <Windows.h>
#include <DirectXMath.h>
#include <memory>

class Font {
public:
	Font() {}

	Font(const Font& rhs) = delete;

	Font& operator==(const Font& rhs) = delete;

	~Font() { if (nullptr != font_) { delete font_; } }
private:
	struct FontType {
		float left_, right_;
		
		int size_;
	};

	struct VertexType {
		DirectX::XMFLOAT3 position_;
		DirectX::XMFLOAT2 texture_position_;
	};
public:
	bool Initialize(WCHAR *font_filename);

	void BuildVertexArray(void* vertices, WCHAR *sentence, float drawX, float drawY);
private:
	bool LoadFontData(WCHAR *font_name);
private:
	FontType *font_ = nullptr;
};

#endif