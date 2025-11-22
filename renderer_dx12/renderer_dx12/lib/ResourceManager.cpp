#include "stdafx.h"
#include "ResourceManager.h"
#include "DirectX12Device.h"

#include <fstream>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <cmath>

using namespace DirectX;

namespace {

struct LegacyVertexType {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT2 texture;
  DirectX::XMFLOAT3 normal;
};

struct PBRVertexType {
  DirectX::XMFLOAT3 position;
  DirectX::XMFLOAT2 texcoord;
  DirectX::XMFLOAT3 normal;
  DirectX::XMFLOAT3 tangent;
  DirectX::XMFLOAT3 binormal;
};

struct ModelDataRaw {
  float x, y, z;
  float tu, tv;
  float nx, ny, nz;
  float tx, ty, tz;
  float bx, by, bz;
};

bool SkipUntil(std::istream &stream, char delimiter) {
  char ch = 0;
  while (stream.get(ch)) {
    if (ch == delimiter) {
      return true;
    }
  }
  return false;
}

bool LoadRawModelData(const std::wstring &filename,
                      std::vector<ModelDataRaw> &data, UINT &vertex_count) {
  std::ifstream fin(filename);
  if (!fin.is_open()) {
    return false;
  }

  if (!SkipUntil(fin, ':')) {
    return false;
  }

  fin >> vertex_count;
  if (!fin) {
    return false;
  }

  data.resize(vertex_count);

  if (!SkipUntil(fin, ':')) {
    return false;
  }
  // Skip newline/whitespace
  fin.get();
  fin.get();

  for (UINT i = 0; i < vertex_count; ++i) {
    fin >> data[i].x >> data[i].y >> data[i].z;
    fin >> data[i].tu >> data[i].tv;
    fin >> data[i].nx >> data[i].ny >> data[i].nz;
    // Initialize tangent/binormal to 0
    data[i].tx = data[i].ty = data[i].tz = 0.0f;
    data[i].bx = data[i].by = data[i].bz = 0.0f;
  }

  return true;
}

void CalculateTangentBinormal(std::vector<ModelDataRaw> &data) {
  size_t vertex_count = data.size();
  size_t face_count = vertex_count / 3;
  size_t index = 0;

  for (size_t i = 0; i < face_count; ++i) {
    auto &v1 = data[index];
    auto &v2 = data[index + 1];
    auto &v3 = data[index + 2];

    float vector1[3], vector2[3];
    float tu_vector[2], tv_vector[2];

    vector1[0] = v2.x - v1.x;
    vector1[1] = v2.y - v1.y;
    vector1[2] = v2.z - v1.z;

    vector2[0] = v3.x - v1.x;
    vector2[1] = v3.y - v1.y;
    vector2[2] = v3.z - v1.z;

    tu_vector[0] = v2.tu - v1.tu;
    tv_vector[0] = v2.tv - v1.tv;

    tu_vector[1] = v3.tu - v1.tu;
    tv_vector[1] = v3.tv - v1.tv;

    float determinant =
        tu_vector[0] * tv_vector[1] - tu_vector[1] * tv_vector[0];
    
    XMFLOAT3 tangent(1.0f, 0.0f, 0.0f);
    XMFLOAT3 binormal(0.0f, 1.0f, 0.0f);

    if (fabsf(determinant) >= 1e-6f) {
      float denominator = 1.0f / determinant;

      tangent.x = (tv_vector[1] * vector1[0] - tv_vector[0] * vector2[0]) *
                  denominator;
      tangent.y = (tv_vector[1] * vector1[1] - tv_vector[0] * vector2[1]) *
                  denominator;
      tangent.z = (tv_vector[1] * vector1[2] - tv_vector[0] * vector2[2]) *
                  denominator;

      binormal.x = (tu_vector[0] * vector2[0] - tu_vector[1] * vector1[0]) *
                   denominator;
      binormal.y = (tu_vector[0] * vector2[1] - tu_vector[1] * vector1[1]) *
                   denominator;
      binormal.z = (tu_vector[0] * vector2[2] - tu_vector[1] * vector1[2]) *
                   denominator;

      XMVECTOR t = XMLoadFloat3(&tangent);
      t = XMVector3Normalize(t);
      XMStoreFloat3(&tangent, t);

      XMVECTOR b = XMLoadFloat3(&binormal);
      b = XMVector3Normalize(b);
      XMStoreFloat3(&binormal, b);
    }

    // Apply to all 3 vertices of the face (flat shading assumption for T/B?)
    // PBRModel implementation applies same T/B to all 3 vertices.
    for (int k = 0; k < 3; ++k) {
        data[index + k].tx = tangent.x;
        data[index + k].ty = tangent.y;
        data[index + k].tz = tangent.z;
        data[index + k].bx = binormal.x;
        data[index + k].by = binormal.y;
        data[index + k].bz = binormal.z;
    }
    
    index += 3;
  }
}

} // namespace

ResourceManager &ResourceManager::GetInstance() {
  static ResourceManager instance;
  return instance;
}

void ResourceManager::Initialize(std::shared_ptr<DirectX12Device> device) {
  device_ = device;
}

void ResourceManager::ClearCache() {
  texture_cache_.clear();
  model_cache_.clear();
}

ResourceSharedPtr ResourceManager::GetTexture(const std::wstring &filename) {
  if (!device_) return nullptr;

  auto it = texture_cache_.find(filename);
  if (it != texture_cache_.end()) {
    return it->second;
  }

  ResourceSharedPtr resource;
  if (device_->LoadTexture(filename, resource)) {
    texture_cache_[filename] = resource;
    return resource;
  }
  return nullptr;
}

std::shared_ptr<ModelResource>
ResourceManager::GetModel(const std::wstring &filename, ModelLoaderType type) {
  if (!device_) return nullptr;

  // Create unique key for cache
  std::wstring key = filename + (type == ModelLoaderType::PBR ? L":PBR" : L":Legacy");
  
  auto it = model_cache_.find(key);
  if (it != model_cache_.end()) {
    return it->second;
  }

  // Load Data
  std::vector<ModelDataRaw> raw_data;
  UINT vertex_count = 0;
  if (!LoadRawModelData(filename, raw_data, vertex_count)) {
    return nullptr;
  }

  // Prepare buffers
  ResourceSharedPtr vertex_buffer;
  ResourceSharedPtr index_buffer;
  size_t vertex_stride = 0;
  size_t vertex_buffer_size = 0;
  size_t index_buffer_size = 0;
  DXGI_FORMAT index_format = DXGI_FORMAT_UNKNOWN;

  if (type == ModelLoaderType::PBR) {
    CalculateTangentBinormal(raw_data);
    
    std::vector<PBRVertexType> vertices(vertex_count);
    std::vector<uint32_t> indices(vertex_count);

    for (UINT i = 0; i < vertex_count; ++i) {
        vertices[i].position = XMFLOAT3(raw_data[i].x, raw_data[i].y, raw_data[i].z);
        vertices[i].texcoord = XMFLOAT2(raw_data[i].tu, raw_data[i].tv);
        vertices[i].normal = XMFLOAT3(raw_data[i].nx, raw_data[i].ny, raw_data[i].nz);
        vertices[i].tangent = XMFLOAT3(raw_data[i].tx, raw_data[i].ty, raw_data[i].tz);
        vertices[i].binormal = XMFLOAT3(raw_data[i].bx, raw_data[i].by, raw_data[i].bz);
        indices[i] = i;
    }

    vertex_stride = sizeof(PBRVertexType);
    vertex_buffer_size = vertex_stride * vertex_count;
    index_buffer_size = sizeof(uint32_t) * vertex_count;
    index_format = DXGI_FORMAT_R32_UINT;
    
    if (!device_->CreateBuffer(vertex_buffer_size, vertices.data(), vertex_buffer, 
                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)) {
        return nullptr;
    }
    if (!device_->CreateBuffer(index_buffer_size, indices.data(), index_buffer,
                               D3D12_RESOURCE_STATE_INDEX_BUFFER)) {
        return nullptr;
    }

  } else {
    // Legacy
    std::vector<LegacyVertexType> vertices(vertex_count);
    std::vector<uint16_t> indices(vertex_count);

    for (UINT i = 0; i < vertex_count; ++i) {
        vertices[i].position = XMFLOAT3(raw_data[i].x, raw_data[i].y, raw_data[i].z);
        vertices[i].texture = XMFLOAT2(raw_data[i].tu, raw_data[i].tv);
        vertices[i].normal = XMFLOAT3(raw_data[i].nx, raw_data[i].ny, raw_data[i].nz);
        indices[i] = static_cast<uint16_t>(i);
    }

    vertex_stride = sizeof(LegacyVertexType);
    vertex_buffer_size = vertex_stride * vertex_count;
    index_buffer_size = sizeof(uint16_t) * vertex_count;
    index_format = DXGI_FORMAT_R16_UINT;

    if (!device_->CreateBuffer(vertex_buffer_size, vertices.data(), vertex_buffer,
                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER)) {
        return nullptr;
    }
    if (!device_->CreateBuffer(index_buffer_size, indices.data(), index_buffer,
                               D3D12_RESOURCE_STATE_INDEX_BUFFER)) {
        return nullptr;
    }
  }

  auto model_res = std::make_shared<ModelResource>();
  model_res->vertex_buffer = vertex_buffer;
  model_res->index_buffer = index_buffer;
  model_res->vertex_count = vertex_count;
  model_res->index_count = vertex_count; // index count == vertex count in this loader
  
  model_res->vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
  model_res->vertex_buffer_view.SizeInBytes = static_cast<UINT>(vertex_buffer_size);
  model_res->vertex_buffer_view.StrideInBytes = static_cast<UINT>(vertex_stride);

  model_res->index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
  model_res->index_buffer_view.SizeInBytes = static_cast<UINT>(index_buffer_size);
  model_res->index_buffer_view.Format = index_format;

  model_cache_[key] = model_res;
  return model_res;
}

