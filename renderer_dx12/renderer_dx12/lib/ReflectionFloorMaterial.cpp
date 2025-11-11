#include "stdafx.h"

#include "ReflectionFloorMaterial.h"

#include "DirectX12Device.h"

using namespace DirectX;

ReflectionFloorMaterial::ReflectionFloorMaterial(
    std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

bool ReflectionFloorMaterial::Initialize() {
  if (!device_) {
    return false;
  }

  auto d3d_device = device_->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  if (FAILED(d3d_device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(MatrixBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&matrix_constant_buffer_)))) {
    return false;
  }

  if (FAILED(d3d_device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(ReflectionBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&reflection_constant_buffer_)))) {
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

ResourceSharedPtr
ReflectionFloorMaterial::GetMatrixConstantBuffer() const {
  return matrix_constant_buffer_;
}

ResourceSharedPtr
ReflectionFloorMaterial::GetReflectionConstantBuffer() const {
  return reflection_constant_buffer_;
}

bool ReflectionFloorMaterial::UpdateMatrixConstant(
    const XMMATRIX &world, const XMMATRIX &view, const XMMATRIX &projection) {
  if (!matrix_constant_buffer_) {
    return false;
  }

  UINT8 *mapped_data = nullptr;
  if (FAILED(matrix_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&mapped_data)))) {
    return false;
  }

  XMStoreFloat4x4(&matrix_data_.world, world);
  XMStoreFloat4x4(&matrix_data_.view, view);
  XMStoreFloat4x4(&matrix_data_.projection, projection);

  memcpy(mapped_data, &matrix_data_, sizeof(MatrixBufferType));
  matrix_constant_buffer_->Unmap(0, nullptr);

  return true;
}

bool ReflectionFloorMaterial::UpdateReflectionConstant(
    const XMMATRIX &reflection) {
  if (!reflection_constant_buffer_) {
    return false;
  }

  UINT8 *mapped_data = nullptr;
  if (FAILED(reflection_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&mapped_data)))) {
    return false;
  }

  XMStoreFloat4x4(&reflection_data_.reflection, reflection);
  memcpy(mapped_data, &reflection_data_, sizeof(ReflectionBufferType));
  reflection_constant_buffer_->Unmap(0, nullptr);

  return true;
}

bool ReflectionFloorMaterial::InitializeRootSignature() {
  if (!device_) {
    return false;
  }

  CD3DX12_DESCRIPTOR_RANGE srv_range;
  srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[3];
  root_parameters[0].InitAsDescriptorTable(1, &srv_range,
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Matrix buffer
  root_parameters[2].InitAsConstantBufferView(
      1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Reflection buffer

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

  CD3DX12_ROOT_SIGNATURE_DESC root_desc(
      _countof(root_parameters), root_parameters, 1, &sampler_desc,
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
          D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);

  Microsoft::WRL::ComPtr<ID3DBlob> signature;
  Microsoft::WRL::ComPtr<ID3DBlob> error;
  if (FAILED(D3D12SerializeRootSignature(&root_desc,
                                         D3D_ROOT_SIGNATURE_VERSION_1,
                                         &signature, &error))) {
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

bool ReflectionFloorMaterial::InitializeGraphicsPipelineState() {
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
       0}};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.InputLayout = {input_desc, _countof(input_desc)};
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

  SetPSOByName("reflection_floor_main", pso);
  return true;
}

