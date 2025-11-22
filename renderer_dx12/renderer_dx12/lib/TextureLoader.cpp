#include "stdafx.h"

#include "DirectX12Device.h"
#include "TextureLoader.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cwctype>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace ResourceLoader {

namespace {

std::wstring ToLower(std::wstring value) {
  std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
    return static_cast<wchar_t>(std::towlower(c));
  });
  return value;
}

bool EndsWith(const std::wstring &value, const std::wstring &suffix) {
  if (value.length() < suffix.length()) {
    return false;
  }
  return value.compare(value.length() - suffix.length(), suffix.length(),
                       suffix) == 0;
}

} // namespace

TextureLoader::TextureLoader(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

bool TextureLoader::LoadTextureByName(WCHAR **texture_filename) {
  return false;
}

bool TextureLoader::LoadTexturesByNameArray(unsigned int num_textures,
                                            WCHAR **texture_filename_arr) {

  auto device = device_->GetD3d12Device();

  D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
  srv_heap_desc.NumDescriptors = num_textures;
  srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  if (FAILED(device->CreateDescriptorHeap(
          &srv_heap_desc, IID_PPV_ARGS(&shader_resource_view_heap_)))) {
    return false;
  }

  auto increasement_size = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
      shader_resource_view_heap_.Get()->GetCPUDescriptorHandleForHeapStart());
  ResourceSharedPtr tem_texture = nullptr;
  auto index = 0;
  string filename = {};

  for (unsigned int i = 0; i < num_textures; ++i) {
    tem_texture.Reset();
    std::wstring file_path(texture_filename_arr[i]);
    
    tem_texture = ResourceManager::GetInstance().GetTexture(file_path);
    if (!tem_texture) {
      return false;
    }

    texture_container_.push_back(tem_texture);

    // Create SRV
    D3D12_RESOURCE_DESC desc = tem_texture->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.Format = desc.Format;
    srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

    device->CreateShaderResourceView(tem_texture.Get(), &srv_desc, handle);

    filename.clear();
    WCHARToString(texture_filename_arr[i], filename);
    index_container_.insert(make_pair(
        filename, static_cast<unsigned int>(texture_container_.size() - 1)));

    handle.Offset(increasement_size);
  }

  return true;
}

ResourceSharedPtr TextureLoader::GetTextureResource(size_t index) const {
  if (index >= texture_container_.size()) {
    return nullptr;
  }
  return texture_container_[index];
}
} // namespace ResourceLoader
