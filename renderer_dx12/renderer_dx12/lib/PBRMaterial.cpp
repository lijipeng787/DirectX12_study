#include "stdafx.h"

#include "PBRMaterial.h"

#include "DirectX12Device.h"
#include "SceneLight.h"

using namespace DirectX;

PBRMaterial::PBRMaterial(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

PBRMaterial::~PBRMaterial() {}

auto PBRMaterial::Initialize() -> bool {
  auto device = device_->GetD3d12Device();

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(MatrixBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&matrix_constant_buffer_)))) {
    return false;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(CameraBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&camera_constant_buffer_)))) {
    return false;
  }

  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(LightBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&light_constant_buffer_)))) {
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

auto PBRMaterial::UpdateMatrixConstant(const XMMATRIX &world,
                                       const XMMATRIX &view,
                                       const XMMATRIX &projection) -> bool {
  UINT8 *data_begin = nullptr;
  if (FAILED(matrix_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  XMStoreFloat4x4(&matrix_data_.world, world);
  XMStoreFloat4x4(&matrix_data_.view, view);
  XMStoreFloat4x4(&matrix_data_.projection, projection);

  // Calculate normal matrix: inverse-transpose of world matrix
  // This ensures correct normal transformation even with non-uniform scaling
  XMVECTOR det;
  XMMATRIX worldInverse = XMMatrixInverse(&det, world);
  XMMATRIX normalMatrix = XMMatrixTranspose(worldInverse);
  XMStoreFloat4x4(&matrix_data_.normalMatrix, normalMatrix);

  memcpy(data_begin, &matrix_data_, sizeof(MatrixBufferType));
  matrix_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto PBRMaterial::UpdateCameraConstant(const XMFLOAT3 &camera_position)
    -> bool {
  UINT8 *data_begin = nullptr;
  if (FAILED(camera_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  camera_data_.camera_position =
      XMFLOAT4(camera_position.x, camera_position.y, camera_position.z, 1.0f);
  memcpy(data_begin, &camera_data_, sizeof(CameraBufferType));
  camera_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto PBRMaterial::UpdateLightConstant(const XMFLOAT3 &light_direction) -> bool {
  UINT8 *data_begin = nullptr;
  if (FAILED(light_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  light_data_.light_direction =
      XMFLOAT4(light_direction.x, light_direction.y, light_direction.z, 0.0f);
  memcpy(data_begin, &light_data_, sizeof(LightBufferType));
  light_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto PBRMaterial::UpdateFromLight(const Lighting::SceneLight *scene_light)
    -> bool {
  if (!scene_light) {
    return false;
  }

  // For PBR, extract light direction and effective color
  // PBR typically uses direction and radiance (color * intensity)
  const auto &direction = scene_light->GetDirection();

  return UpdateLightConstant(direction);
}

auto PBRMaterial::GetMatrixConstantBuffer() const -> ResourceSharedPtr {
  return matrix_constant_buffer_;
}

auto PBRMaterial::GetCameraConstantBuffer() const -> ResourceSharedPtr {
  return camera_constant_buffer_;
}

auto PBRMaterial::GetLightConstantBuffer() const -> ResourceSharedPtr {
  return light_constant_buffer_;
}

auto PBRMaterial::InitializeRootSignature() -> bool {
  CD3DX12_DESCRIPTOR_RANGE descriptor_range;
  descriptor_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[4];
  root_parameters[0].InitAsDescriptorTable(1, &descriptor_range,
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(
      0, 0,
      D3D12_SHADER_VISIBILITY_VERTEX); // VS: MatrixBuffer (b0)
  root_parameters[2].InitAsConstantBufferView(
      1, 0,
      D3D12_SHADER_VISIBILITY_VERTEX); // VS: CameraBuffer (b1)
  root_parameters[3].InitAsConstantBufferView(
      2, 0,
      D3D12_SHADER_VISIBILITY_PIXEL); // PS: LightBuffer (b2) - global unique
                                      // numbering

  D3D12_STATIC_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler_desc.MipLODBias = 0.0f;
  sampler_desc.MaxAnisotropy = 1;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
  sampler_desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  sampler_desc.ShaderRegister = 0;
  sampler_desc.RegisterSpace = 0;
  sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(
      _countof(root_parameters), root_parameters, 1, &sampler_desc,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

  Microsoft::WRL::ComPtr<ID3DBlob> signature;
  Microsoft::WRL::ComPtr<ID3DBlob> error;
  if (FAILED(D3D12SerializeRootSignature(
          &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1,
          signature.GetAddressOf(), error.GetAddressOf()))) {
    return false;
  }

  RootSignaturePtr root_signature = nullptr;
  if (FAILED(device_->GetD3d12Device()->CreateRootSignature(
          0, signature->GetBufferPointer(), signature->GetBufferSize(),
          IID_PPV_ARGS(&root_signature)))) {
    return false;
  }

  SetRootSignature(root_signature);
  return true;
}

auto PBRMaterial::InitializeGraphicsPipelineState() -> bool {
  D3D12_INPUT_ELEMENT_DESC input_layout[] = {
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

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.InputLayout = {input_layout, _countof(input_layout)};
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

  PipelineStateObjectPtr pso = nullptr;
  if (FAILED(device_->GetD3d12Device()->CreateGraphicsPipelineState(
          &pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }

  SetPSOByName("pbr_pipeline", pso);

  return true;
}
