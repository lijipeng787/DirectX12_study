#include "stdafx.h"

#include "ModelMaterial.h"

#include "DirectX12Device.h"
#include "SceneLight.h"

using namespace DirectX;

auto ModelMaterial::Initialize() -> bool {

  auto device = device_->GetD3d12Device();

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(CBSIZE(MatrixBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&matrix_constant_buffer_)))) {
    return false;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(CBSIZE(LightType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&light_constant_buffer_)))) {
    return false;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(CBSIZE(FogBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&fog_constant_buffer_)))) {
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

  UINT8 *data_begin = nullptr;
  if (FAILED(matrix_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  const auto original_world = XMMatrixTranspose(world);
  const auto inv_world = XMMatrixInverse(nullptr, original_world);
  const auto normal_matrix = inv_world;

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, view);
  XMStoreFloat4x4(&matrix_constant_data_.projection_, projection);
  XMStoreFloat4x4(&matrix_constant_data_.normal_, normal_matrix);

  memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
  matrix_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto ModelMaterial::UpdateLightConstant(const XMFLOAT4 &ambient_color,
                                        const XMFLOAT4 &diffuse_color,
                                        const XMFLOAT3 &direction) -> bool {

  UINT8 *data_begin = nullptr;
  if (FAILED(light_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  light_constant_data_.ambient_color_ = ambient_color;
  light_constant_data_.diffuse_color_ = diffuse_color;
  light_constant_data_.direction_ = direction;
  light_constant_data_.padding_ = 0.0f;

  memcpy(data_begin, &light_constant_data_, sizeof(LightType));
  light_constant_buffer_->Unmap(0, nullptr);

  return true;
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

  UINT8 *data_begin = nullptr;
  if (FAILED(fog_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  fog_constant_data_.fog_start_ = fog_begin;
  fog_constant_data_.fog_end_ = fog_end;

  memcpy(data_begin, &fog_constant_data_, sizeof(FogBufferType));
  fog_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto ModelMaterial::InitializeRootSignature() -> bool {

  CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
  descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[4];
  root_parameters[0].InitAsDescriptorTable(1, descriptor_ranges_srv,
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  root_parameters[2].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[3].InitAsConstantBufferView(
      1, 0, D3D12_SHADER_VISIBILITY_VERTEX);

  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.MipLODBias = 0;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc.ShaderRegister = 0;
  sampler_desc.RegisterSpace = 0;
  sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  CD3DX12_ROOT_SIGNATURE_DESC rootsignature_layout(
      ARRAYSIZE(root_parameters), root_parameters, 1, &sampler_desc,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

  ID3DBlob *signature_blob = nullptr;
  ID3DBlob *signature_error = nullptr;
  if (FAILED(D3D12SerializeRootSignature(&rootsignature_layout,
                                         D3D_ROOT_SIGNATURE_VERSION_1,
                                         &signature_blob, &signature_error))) {
    return false;
  }

  RootSignaturePtr root_signature = {};
  if (FAILED(device_->GetD3d12Device()->CreateRootSignature(
          0, signature_blob->GetBufferPointer(),
          signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)))) {
    return false;
  }
  SetRootSignature(root_signature);

  return true;
}

auto ModelMaterial::InitializeGraphicsPipelineState() -> bool {

  D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.InputLayout = {input_element_descs, _countof(input_element_descs)};
  pso_desc.pRootSignature = GetRootSignature().Get();
  pso_desc.VS = GetVSByteCode();
  pso_desc.PS = GetPSByteCode();
  pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  pso_desc.SampleDesc.Count = 1;

  PipelineStateObjectPtr pso = {};
  if (FAILED(device_->GetD3d12Device()->CreateGraphicsPipelineState(
          &pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }
  SetPSOByName("model_normal", pso);

  return true;
}


