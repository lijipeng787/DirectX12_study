#include "stdafx.h"

#include "BumpMapModel.h"

#include "DirectX12Device.h"

using namespace ResourceLoader;

BumpMapModel::BumpMapModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

BumpMapModel::~BumpMapModel() {}

auto BumpMapModel::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                              unsigned int texture_count) -> bool {
  model_resource_ = ResourceManager::GetInstance().GetModel(model_filename, ModelLoaderType::PBR);
  if (!model_resource_) {
    return false;
  }

  if (!LoadTextures(texture_filename_arr, texture_count)) {
    return false;
  }

  if (!material_.Initialize()) {
    return false;
  }

  return true;
}

auto BumpMapModel::GetShaderResourceView() const -> DescriptorHeapPtr {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTexturesDescriptorHeap();
}

auto BumpMapModel::LoadTextures(WCHAR **texture_filename_arr,
                                unsigned int texture_count) -> bool {
  texture_loader_ = std::make_shared<TextureLoader>(device_);
  return texture_loader_->LoadTexturesByNameArray(texture_count,
                                                  texture_filename_arr);
}
