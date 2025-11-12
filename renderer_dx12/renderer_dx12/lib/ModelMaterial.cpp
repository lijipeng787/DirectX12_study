#include "stdafx.h"

#include "ModelMaterial.h"

#include "DirectX12Device.h"
#include "PipelineStateBuilder.h"
#include "RootSignatureBuilder.h"
#include "SceneLight.h"

using namespace DirectX;

auto ModelMaterial::Initialize() -> bool {

  if (!device_) {
    return false;
  }

  if (!matrix_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!light_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!fog_constant_buffer_.Initialize(device_)) {
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

auto ModelMaterial::UpdateMatrixConstant(const XMMATRIX &world,
                                         const XMMATRIX &view,
                                         const XMMATRIX &projection) -> bool {

  const auto original_world = XMMatrixTranspose(world);
  const auto inv_world = XMMatrixInverse(nullptr, original_world);
  const auto normal_matrix = inv_world;

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, view);
  XMStoreFloat4x4(&matrix_constant_data_.projection_, projection);
  XMStoreFloat4x4(&matrix_constant_data_.normal_, normal_matrix);

  return matrix_constant_buffer_.Update(matrix_constant_data_);
}

auto ModelMaterial::UpdateLightConstant(const XMFLOAT4 &ambient_color,
                                        const XMFLOAT4 &diffuse_color,
                                        const XMFLOAT3 &direction) -> bool {

  light_constant_data_.ambient_color_ = ambient_color;
  light_constant_data_.diffuse_color_ = diffuse_color;
  light_constant_data_.direction_ = direction;
  light_constant_data_.padding_ = 0.0f;

  return light_constant_buffer_.Update(light_constant_data_);
}

auto ModelMaterial::UpdateFromLight(
    const Lighting::SceneLight *scene_light) -> bool {
  if (scene_light == nullptr) {
    return false;
  }

  return UpdateLightConstant(scene_light->GetAmbientColor(),
                             scene_light->GetDiffuseColor(),
                             scene_light->GetDirection());
}

auto ModelMaterial::UpdateFogConstant(float fog_begin, float fog_end) -> bool {

  fog_constant_data_.fog_start_ = fog_begin;
  fog_constant_data_.fog_end_ = fog_end;

  return fog_constant_buffer_.Update(fog_constant_data_);
}

auto ModelMaterial::InitializeRootSignature() -> bool {

  if (!device_) {
    return false;
  }

  RootSignatureBuilder builder;
  builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                             D3D12_SHADER_VISIBILITY_PIXEL);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  builder.AddConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
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

auto ModelMaterial::InitializeGraphicsPipelineState() -> bool {

  if (!device_) {
    return false;
  }

  D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0}};

  GraphicsPipelineStateBuilder builder;
  builder.SetRootSignature(GetRootSignature());
  builder.SetVertexShader(GetVSByteCode());
  builder.SetPixelShader(GetPSByteCode());
  builder.SetInputLayout(input_element_descs, _countof(input_element_descs));
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
  SetPSOByName("model_normal", pso);

  return true;
}


