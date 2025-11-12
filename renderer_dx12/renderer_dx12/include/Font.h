#pragma once

#include <DirectXMath.h>
#include <Windows.h>

class BitmapFont {
public:
  BitmapFont() = default;

  BitmapFont(const BitmapFont &rhs) = delete;

  auto operator=(const BitmapFont &rhs) -> BitmapFont & = delete;

  ~BitmapFont() {
      delete[] font_;
  }

private:
  struct FontType {
    float left_ = 0.0f;
    float right_ = 0.0f;
    int width_ = 0;
  };

  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
  };

public:
  auto Initialize(WCHAR *font_filename) -> bool;

  void BuildVertexArray(void *vertices, WCHAR *sentence, float drawX,
                        float drawY);

private:
  auto LoadFontData(WCHAR *font_name) -> bool;

  FontType *font_ = nullptr;
};
