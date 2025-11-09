#ifndef HEADER_TEXTURECLASS_H
#define HEADER_TEXTURECLASS_H

#include <memory>
#include <unordered_map>

#include "TypeDefine.h"

class DirectX12Device;

namespace ResourceLoader {

typedef std::vector<ResourceSharedPtr> TextureContainer;

typedef std::unordered_map<std::string, unsigned int> TextureIndexContainer;

class TextureLoader {
public:
  explicit TextureLoader(std::shared_ptr<DirectX12Device> device);

  TextureLoader(const TextureLoader &rhs) = delete;

  TextureLoader &operator=(const TextureLoader &rhs) = delete;

  ~TextureLoader() {}

public:
  DescriptorHeapPtr GetTexturesDescriptorHeap() const {
    return shader_resource_view_heap_;
  }

  bool LoadTextureByName(WCHAR **texture_filename);

  bool LoadTexturesByNameArray(unsigned int num_textures,
                               WCHAR **texture_filename_arr);

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  unsigned int num_textures_ = 0;

  TextureContainer texture_container_ = {};

  TextureIndexContainer index_container_ = {};

  DescriptorHeapPtr shader_resource_view_heap_ = nullptr;
};
} // namespace ResourceLoader

#endif // !HEADER_TEXTURECLASS_H
