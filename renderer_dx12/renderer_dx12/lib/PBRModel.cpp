#include "stdafx.h"

#include "PBRModel.h"

#include <utility>

#include "DirectX12Device.h"

using namespace ResourceLoader;

PBRModel::PBRModel(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)), material_(device_) {}

PBRModel::~PBRModel() {}

auto PBRModel::Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) -> bool {
  model_resource_ = ResourceManager::GetInstance().GetModel(model_filename, ModelLoaderType::PBR);
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

auto PBRModel::GetMaterial() -> PBRMaterial * { return &material_; }

auto PBRModel::GetShaderResourceView() const -> DescriptorHeapPtr {
  if (!texture_container_) {
    return nullptr;
  }
  return texture_container_->GetTexturesDescriptorHeap();
}

auto PBRModel::LoadTexture(WCHAR **texture_filename_arr) -> bool {
  texture_container_ = std::make_shared<TextureLoader>(device_);
  return texture_container_->LoadTexturesByNameArray(3, texture_filename_arr);
}
