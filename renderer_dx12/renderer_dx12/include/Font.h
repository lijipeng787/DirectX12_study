#pragma once

#include <DirectXMath.h>
#include <Windows.h>

class BitmapFont {
public:
  BitmapFont() = default;

  BitmapFont(const BitmapFont &rhs) = delete;

  BitmapFont &operator=(const BitmapFont &rhs) = delete;

  ~BitmapFont() {
    if (nullptr != font_) {
      delete[] font_;
    }
  }

private:
  struct FontType {
    float left_, right_;
    int width_;
  };

  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
  };

public:
  bool Initialize(WCHAR *font_filename);

  void BuildVertexArray(void *vertices, WCHAR *sentence, float drawX,
                        float drawY);

private:
  bool LoadFontData(WCHAR *font_name);

private:
  FontType *font_ = nullptr;
};
