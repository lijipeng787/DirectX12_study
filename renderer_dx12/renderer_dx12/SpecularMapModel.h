#pragma once

#include <DirectXMath.h>
#include <memory>

#include "SpecularMapMaterial.h"
#include "TextureLoader.h"

class DirectX12Device;

class SpecularMapModel {
public:
  explicit SpecularMapModel(std::shared_ptr<DirectX12Device> device);

  SpecularMapModel(const SpecularMapModel &rhs) = delete;
  auto operator=(const SpecularMapModel &rhs) -> SpecularMapModel & = delete;

  ~SpecularMapModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr,
                  unsigned int texture_count) -> bool;

  auto GetIndexCount() const -> UINT { return index_count_; }

  auto GetVertexBufferView() const -> const D3D12_VERTEX_BUFFER_VIEW & {
    return vertex_buffer_view_;
  }

  auto GetIndexBufferView() const -> const D3D12_INDEX_BUFFER_VIEW & {
    return index_buffer_view_;
  }

  auto GetMaterial() -> SpecularMapMaterial * { return &material_; }

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

private:
  struct VertexType {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT2 texcoord;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 tangent;
    DirectX::XMFLOAT3 binormal;
  };

  struct ModelType {
    float x, y, z;
    float tu, tv;
    float nx, ny, nz;
    float tx, ty, tz;
    float bx, by, bz;
  };

  struct TempVertexType {
    float x, y, z;
    float tu, tv;
    float nx, ny, nz;
  };

  struct VectorType {
    float x, y, z;
  };

  auto LoadModel(WCHAR *filename) -> bool;
  void ReleaseModel();

  auto InitializeBuffers() -> bool;
  auto LoadTextures(WCHAR **texture_filename_arr, unsigned int texture_count)
      -> bool;

  void CalculateModelVectors();
  void CalculateTangentBinormal(const TempVertexType &vertex1,
                                const TempVertexType &vertex2,
                                const TempVertexType &vertex3,
                                VectorType &tangent, VectorType &binormal);
  void CalculateNormal(const VectorType &tangent, const VectorType &binormal,
                       VectorType &normal);

private:
  std::shared_ptr<DirectX12Device> device_;
  SpecularMapMaterial material_;

  ResourceSharedPtr vertex_buffer_ = nullptr;
  ResourceSharedPtr index_buffer_ = nullptr;

  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};
  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT vertex_count_ = 0;
  UINT index_count_ = 0;

  ModelType *model_data_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_loader_ = nullptr;
};

