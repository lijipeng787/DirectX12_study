#include "stdafx.h"

#include "SpecularMapMaterial.h"

#include "DirectX12Device.h"
#include "PipelineStateBuilder.h"
#include "RootSignatureBuilder.h"
#include "SceneLight.h"

using namespace DirectX;

SpecularMapMaterial::SpecularMapMaterial(
    std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

auto SpecularMapMaterial::Initialize() -> bool {
  if (!device_) {
    return false;
  }

  if (!matrix_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!camera_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!light_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!InitializeRootSignature()) {
    return false;
  }

  if (!InitializeGraphicsPipelineState()) {
    return false;
  }

  return true;
}

auto SpecularMapMaterial::GetMatrixConstantBuffer() const -> ResourceSharedPtr {
  return matrix_constant_buffer_.GetResource();
}

auto SpecularMapMaterial::GetCameraConstantBuffer() const -> ResourceSharedPtr {
  return camera_constant_buffer_.GetResource();
}

auto SpecularMapMaterial::GetLightConstantBuffer() const -> ResourceSharedPtr {
  return light_constant_buffer_.GetResource();
}

auto SpecularMapMaterial::UpdateMatrixConstant(const XMMATRIX &world,
                                               const XMMATRIX &view,
                                               const XMMATRIX &projection)
    -> bool {
  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, view);
  XMStoreFloat4x4(&matrix_constant_data_.projection_, projection);

  return matrix_constant_buffer_.Update(matrix_constant_data_);
}

auto SpecularMapMaterial::UpdateCameraConstant(
    const XMFLOAT3 &camera_position) -> bool {
  camera_constant_data_.camera_position_ = camera_position;
  camera_constant_data_.padding_ = 0.0f;

  return camera_constant_buffer_.Update(camera_constant_data_);
}

auto SpecularMapMaterial::UpdateLightConstant(
    const XMFLOAT4 &diffuse_color, const XMFLOAT4 &specular_color,
    const XMFLOAT3 &light_direction, float specular_power) -> bool {
  light_constant_data_.diffuse_color_ = diffuse_color;
  light_constant_data_.specular_color_ = specular_color;
  light_constant_data_.light_direction_ = light_direction;
  light_constant_data_.specular_power_ = specular_power;

  return light_constant_buffer_.Update(light_constant_data_);
}

auto SpecularMapMaterial::UpdateLightFromScene(
    const Lighting::SceneLight *scene_light) -> bool {
  if (!scene_light) {
    return false;
  }

  return UpdateLightConstant(scene_light->GetDiffuseColor(),
                             scene_light->GetSpecularColor(),
                             scene_light->GetDirection(),
                             scene_light->GetSpecularPower());
}

auto SpecularMapMaterial::InitializeRootSignature() -> bool {
  if (!device_) {
    return false;
  }

  RootSignatureBuilder builder;
  builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0,
                             D3D12_SHADER_VISIBILITY_PIXEL);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  builder.AddConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc.ShaderRegister = 0;
  sampler_desc.RegisterSpace = 0;
  sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  builder.AddStaticSampler(sampler_desc);

  RootSignaturePtr root_signature = nullptr;
  if (!builder.Build(
          device_, root_signature,
          D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
              D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
              D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
              D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS)) {
    return false;
  }

  SetRootSignature(root_signature);
  return true;
}

auto SpecularMapMaterial::InitializeGraphicsPipelineState() -> bool {
  if (!device_) {
    return false;
  }

  D3D12_INPUT_ELEMENT_DESC input_desc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0}};

  GraphicsPipelineStateBuilder builder;
  builder.SetRootSignature(GetRootSignature());
  builder.SetVertexShader(GetVSByteCode());
  builder.SetPixelShader(GetPSByteCode());
  builder.SetInputLayout(input_desc, _countof(input_desc));
  builder.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

  const DXGI_FORMAT rtv_formats[] = {DXGI_FORMAT_R8G8B8A8_UNORM};
  builder.SetRenderTargetFormats(_countof(rtv_formats), rtv_formats,
                                 DXGI_FORMAT_D32_FLOAT);
  builder.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
  builder.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
  builder.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));

  PipelineStateObjectPtr pso = nullptr;
  if (!builder.Build(device_, pso)) {
    return false;
  }

  SetPSOByName("specular_map_main", pso);
  return true;
}

