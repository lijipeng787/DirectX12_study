#pragma once

#include "TypeDefine.h"
#include <memory>
#include <unordered_map>
#include <string>

class DirectX12Device;

struct ModelResource {
  ResourceSharedPtr vertex_buffer;
  ResourceSharedPtr index_buffer;
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
  D3D12_INDEX_BUFFER_VIEW index_buffer_view;
  UINT vertex_count;
  UINT index_count;
};

enum class ModelLoaderType {
    Legacy,
    PBR // Includes Tangent/Binormal calculation
};

class ResourceManager {
public:
  static ResourceManager &GetInstance();

  void Initialize(std::shared_ptr<DirectX12Device> device);

  // Get or Load a texture (resource only, no SRV)
  ResourceSharedPtr GetTexture(const std::wstring &filename);

  // Get or Load a model geometry
  std::shared_ptr<ModelResource> GetModel(const std::wstring &filename, ModelLoaderType type = ModelLoaderType::Legacy);

  void ClearCache();
  
  // Debug/Stats
  size_t GetTextureCount() const { return texture_cache_.size(); }
  size_t GetModelCount() const { return model_cache_.size(); }

private:
  ResourceManager() = default;
  ~ResourceManager() = default;
  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;

  std::shared_ptr<DirectX12Device> device_ = nullptr;
  std::unordered_map<std::wstring, ResourceSharedPtr> texture_cache_;
  
  // Key: filename + type. We can use a string key like "Legacy:filename"
  std::unordered_map<std::wstring, std::shared_ptr<ModelResource>> model_cache_;
};

