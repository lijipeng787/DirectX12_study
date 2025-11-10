#pragma once

#include <DirectXMath.h>
#include <memory>

#include "Material.h"
#include "TextureLoader.h"

class DirectX12Device;

namespace Lighting {
class SceneLight;
}

class ModelMaterial : public Effect::Material {
public:
  explicit ModelMaterial(std::shared_ptr<DirectX12Device> device);

  ModelMaterial(const ModelMaterial &rhs) = delete;

  auto operator=(const ModelMaterial &rhs) -> ModelMaterial & = delete;

  virtual ~ModelMaterial() override;

private:
  struct MatrixBufferType {
    DirectX::XMFLOAT4X4 world_;
    DirectX::XMFLOAT4X4 view_;
    DirectX::XMFLOAT4X4 projection_;
    DirectX::XMFLOAT4X4 normal_; // Add normal matrix for correct lighting with non-uniform scaling
  };

  struct LightType {
    DirectX::XMFLOAT4 ambient_color_;
    DirectX::XMFLOAT4 diffuse_color_;
    DirectX::XMFLOAT3 direction_;
    float padding_;
  };

  struct FogBufferType {
    float fog_start_;
    float fog_end_;
    float padding1_;
    float padding2_;
  };

public:
  auto Initialize() -> bool override;

  ResourceSharedPtr GetMatrixConstantBuffer() const;

  ResourceSharedPtr GetLightConstantBuffer() const;

  ResourceSharedPtr GetFogConstantBuffer() const;

  auto UpdateMatrixConstant(const DirectX::XMMATRIX &world,
                            const DirectX::XMMATRIX &view,
                            const DirectX::XMMATRIX &projection) -> bool;

  auto UpdateLightConstant(const DirectX::XMFLOAT4 &ambient_color,
                           const DirectX::XMFLOAT4 &diffuse_color,
                           const DirectX::XMFLOAT3 &direction) -> bool;

  // New unified light interface - extract parameters from SceneLight
  auto UpdateFromLight(const Lighting::SceneLight *scene_light) -> bool;

  auto UpdateFogConstant(float fog_begin, float fog_end) -> bool;

private:
  auto InitializeRootSignature() -> bool;

  auto InitializeGraphicsPipelineState() -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ResourceSharedPtr matrix_constant_buffer_ = nullptr;

  MatrixBufferType matrix_constant_data_ = {};

  ResourceSharedPtr light_constant_buffer_ = nullptr;

  LightType light_constant_data_ = {};

  ResourceSharedPtr fog_constant_buffer_ = nullptr;

  FogBufferType fog_constant_data_ = {};
};

class Model {
public:
  explicit Model(std::shared_ptr<DirectX12Device> device);

  Model(const Model &rhs) = delete;

  auto operator=(const Model &rhs) -> Model & = delete;

  ~Model() {}

private:
  struct VertexType {
    DirectX::XMFLOAT3 position_;
    DirectX::XMFLOAT2 texture_position_;
    DirectX::XMFLOAT3 normal_;
  };

  struct ModelType {
    float x_, y_, z_;
    float tu_, tv_;
    float nx_, ny_, nz_;
  };

public:
  bool Initialize(WCHAR *model_filename, WCHAR **texture_filename_arr);

  UINT GetIndexCount() const { return index_count_; }

  ModelMaterial *GetMaterial();

  DescriptorHeapPtr GetShaderRescourceView() const;

  const D3D12_VERTEX_BUFFER_VIEW &GetVertexBufferView() const {
    return vertex_buffer_view_;
  }

  const D3D12_INDEX_BUFFER_VIEW &GetIndexBufferView() const {
    return index_buffer_view_;
  }

private:
  auto LoadModel(WCHAR *filename) -> bool;

  auto InitializeBuffers() -> bool;

  auto LoadTexture(WCHAR **texture_filename_arr) -> bool;

  std::shared_ptr<DirectX12Device> device_ = nullptr;

  ModelMaterial material_;

  ResourceSharedPtr vertex_buffer_ = nullptr;

  D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_ = {};

  ResourceSharedPtr index_buffer_ = nullptr;

  D3D12_INDEX_BUFFER_VIEW index_buffer_view_ = {};

  UINT vertex_count_ = 0;

  UINT index_count_ = 0;

  ModelType *tem_model_ = nullptr;

  std::shared_ptr<ResourceLoader::TextureLoader> texture_container_ = nullptr;
};
