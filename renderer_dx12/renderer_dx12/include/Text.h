#pragma once

#include <DirectXMath.h>
#include <memory>

#include "TextMaterial.h"
#include "TextureLoader.h"

class BitmapFont;
class DirectX12Device;

class Text {
public:
  explicit Text(std::shared_ptr<DirectX12Device> device);

  Text(const Text &copy) = delete;

  Text &operator=(const Text &rhs) = delete;

  virtual ~Text();

private:
  struct SentenceType {
    ResourceSharedPtr vertex_buffer_ = nullptr;

    D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

    ResourceSharedPtr index_buffer_ = nullptr;

    D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

    D3D12_RESOURCE_STATES vertex_buffer_state_ =
        D3D12_RESOURCE_STATE_COMMON;
    D3D12_RESOURCE_STATES index_buffer_state_ =
        D3D12_RESOURCE_STATE_COMMON;

    int vertex_count_ = 0, index_count_ = 0, max_length_ = 0;
    float red_ = 0.0f, green_ = 0.0f, blue_ = 0.0f;
  };

  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
  };

public:
  bool SetFps(int fps);

  bool SetCpu(int cpu_percentage_value);

public:
  bool Initialize(int screenWidth, int screenHeight,
                  const DirectX::XMMATRIX &baseViewMatrix);

  bool LoadFont(WCHAR *font_data, WCHAR **font_texture);

  TextMaterial *GetMaterial();

  DescriptorHeapPtr GetShaderRescourceView() const;

  UINT GetIndexCount(int index) const {
    return sentence_vector_.at(index)->index_count_;
  }

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView(int index) const {
    return sentence_vector_.at(index)->vertex_buffer_view_;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView(int index) const {
    return sentence_vector_.at(index)->index_buffer_view_;
  }

  unsigned int GetSentenceCount() const {
    return static_cast<unsigned int>(sentence_vector_.size());
  }

  bool UpdateSentenceVertexBuffer(SentenceType *sentence, WCHAR *text,
                                  int positionX, int positionY, float red,
                                  float green, float blue);

private:
  bool InitializeSentence(SentenceType **sentence, int max_length);

  bool LoadTexture(WCHAR **filename_arr);

  void ReleaseSentences();

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  TextMaterial material_;

  std::shared_ptr<BitmapFont> font_ = nullptr;

  int screen_width_ = 0, screen_height_ = 0;

  DirectX::XMMATRIX base_view_matrix_ = {};

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;

  std::vector<SentenceType *> sentence_vector_ = {};
};
