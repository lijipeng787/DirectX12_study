#pragma once

#include <DirectXMath.h>
#include <memory>

#include "PBRMaterial.h"
#include "TextureLoader.h"

class DirectX12Device;

class PBRModel {
public:
  explicit PBRModel(std::shared_ptr<DirectX12Device> device);

  PBRModel(const PBRModel &rhs) = delete;

  auto operator=(const PBRModel &rhs) -> PBRModel & = delete;

  ~PBRModel();

  auto Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr) -> bool;

  auto GetIndexCount() const -> UINT { return index_count_; }

  auto GetMaterial() -> PBRMaterial *;

  auto GetShaderResourceView() const -> DescriptorHeapPtr;

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const {
    return vertex_buffer_view_;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const {
    return index_buffer_view_;
  }

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
  };

  struct VectorType {
    float x, y, z;
  };

  auto LoadModel(WCHAR *filename) -> bool;

  void ReleaseModel();

  void CalculateModelVectors();

  void CalculateTangentBinormal(const TempVertexType &vertex1,
                                const TempVertexType &vertex2,
                                const TempVertexType &vertex3,
                                VectorType &tangent, VectorType &binormal);

  auto InitializeBuffers() -> bool;

  auto LoadTexture(WCHAR **texture_filename_arr) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  PBRMaterial material_;

  ResourceSharedPtr vertex_buffer_ = nullptr;
  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

  ResourceSharedPtr index_buffer_ = nullptr;
  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT vertex_count_ = 0;
  
  UINT index_count_ = 0;

  ModelType *model_data_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;
};

