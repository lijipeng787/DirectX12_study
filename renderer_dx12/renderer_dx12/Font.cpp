#include "stdafx.h"

#include "Font.h"

#include "TypeDefine.h"
#include <fstream>

using namespace std;

bool Font::Initialize(WCHAR *font_filename) {

  if (CHECK(LoadFontData(font_filename))) {
    return false;
  }

  return true;
}

bool Font::LoadFontData(WCHAR *filename) {

  font_ = new FontType[95];
  if (!font_) {
    return false;
  }

  wifstream fin;
  fin.open(filename);
  if (fin.fail()) {
    return false;
  }

  TCHAR temp = ' ';

  for (auto i = 0; i < 95; i++) {
    fin.get(temp);
    while (temp != ' ') {
      fin.get(temp);
    }
    fin.get(temp);
    while (temp != ' ') {
      fin.get(temp);
    }

    fin >> font_[i].left_;
    fin >> font_[i].right_;
    fin >> font_[i].size_;
  }

  fin.close();

  return true;
}

void Font::BuildVertexArray(void *vertices, WCHAR *sentence, float drawX,
                            float drawY) {

  VertexType *vertexPtr = (VertexType *)vertices;

  unsigned int numLetters = lstrlenW(sentence);
  unsigned int index = 0;
  unsigned int letter = 0;

  for (unsigned int i = 0; i < numLetters; ++i) {

    letter = static_cast<int>(sentence[i]) - 32;

    if (0 == letter) {
      drawX = drawX + 3.0f;
    } else {
      // First triangle in quad.
      // Top left.
      vertexPtr[index].position_ = DirectX::XMFLOAT3(drawX, drawY, 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].left_, 0.0f);
      ++index;

      // Bottom right.
      vertexPtr[index].position_ =
          DirectX::XMFLOAT3((drawX + font_[letter].size_), (drawY - 16), 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].right_, 1.0f);
      ++index;

      // Bottom left.
      vertexPtr[index].position_ = DirectX::XMFLOAT3(drawX, (drawY - 16), 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].left_, 1.0f);
      ++index;

      // Second triangle in quad.
      // Top left.
      vertexPtr[index].position_ = DirectX::XMFLOAT3(drawX, drawY, 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].left_, 0.0f);
      ++index;

      // Top right.
      vertexPtr[index].position_ =
          DirectX::XMFLOAT3(drawX + font_[letter].size_, drawY, 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].right_, 0.0f);
      ++index;

      // Bottom right.
      vertexPtr[index].position_ =
          DirectX::XMFLOAT3((drawX + font_[letter].size_), (drawY - 16), 0.0f);
      vertexPtr[index].texture_position_ =
          DirectX::XMFLOAT2(font_[letter].right_, 1.0f);
      ++index;

      // Update the x location for drawing by the size of the letter and one
      // pixel.
      drawX = drawX + font_[letter].size_ + 1.0f;
    }
  }
}
