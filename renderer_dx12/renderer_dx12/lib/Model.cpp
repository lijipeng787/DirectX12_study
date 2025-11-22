#include "stdafx.h"

#include "Model.h"

#include <utility>
#include <vector>

#include "DirectX12Device.h"
#include "ModelMaterial.h"

using namespace DirectX;
using namespace ResourceLoader;

bool Model::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) {
  model_resource_ = ResourceManager::GetInstance().GetModel(model_filename, ModelLoaderType::Legacy);
  if (!model_resource_) {
    return false;
  }

  if (!LoadTexture(texture_filename_arr)) {
    return false;
  }
  if (!material_.Initialize()) {
    return false;
  }

  return true;
}

DescriptorHeapPtr Model::GetShaderResourceView() const {
  if (!texture_container_) {
    return {};
  }
  return texture_container_->GetTexturesDescriptorHeap();
}

bool Model::LoadTexture(WCHAR **texture_filename_arr) {
  texture_container_ = std::make_shared<TextureLoader>(device_);
  constexpr UINT kTextureCount = 3;
  return texture_container_->LoadTexturesByNameArray(kTextureCount, texture_filename_arr);
}
