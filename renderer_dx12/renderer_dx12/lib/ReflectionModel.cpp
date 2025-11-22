#include "stdafx.h"

#include "ReflectionModel.h"

#include <utility>

#include "DirectX12Device.h"

using namespace ResourceLoader;

ReflectionModel::ReflectionModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

ReflectionModel::~ReflectionModel() {}

auto ReflectionModel::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                                 unsigned int texture_count) -> bool {
  model_resource_ = ResourceManager::GetInstance().GetModel(model_filename, ModelLoaderType::Legacy);
  if (!model_resource_) {
    return false;
  }

  if (!LoadTextures(texture_filename_arr, texture_count)) {
    return false;
  }

  return true;
}

DescriptorHeapPtr ReflectionModel::GetShaderResourceView() const {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTexturesDescriptorHeap();
}

ResourceSharedPtr ReflectionModel::GetTextureResource(size_t index) const {
  if (!texture_loader_) {
    return nullptr;
  }
  return texture_loader_->GetTextureResource(index);
}

auto ReflectionModel::LoadTextures(WCHAR **texture_filename_arr,
                                   unsigned int texture_count) -> bool {
  texture_loader_ = std::make_shared<TextureLoader>(device_);
  return texture_loader_->LoadTexturesByNameArray(texture_count,
                                                  texture_filename_arr);
}
