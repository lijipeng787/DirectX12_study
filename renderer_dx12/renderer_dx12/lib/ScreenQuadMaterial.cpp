#include "stdafx.h"

#include "ScreenQuadMaterial.h"

#include "DirectX12Device.h"

using namespace DirectX;

auto ScreenQuadMaterial::Initialize() -> bool {

  if (initialized_) {
    return true;
  }

  D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc = {};
  cbv_heap_desc.NumDescriptors = 1;
  cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

  auto device = device_->GetD3d12Device();
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(CBSIZE(MatrixBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&constant_buffer_)))) {
    return false;
  }

  ZeroMemory(&matrix_constant_data_, sizeof(MatrixBufferType));

  if (!InitializeRootSignature()) {
    return false;
  }

  if (!InitializeGraphicsPipelineState()) {
    return false;
  }

  initialized_ = true;
  return true;
}

auto ScreenQuadMaterial::UpdateConstantBuffer(const XMMATRIX &world,
                                              const XMMATRIX &view,
                                              const XMMATRIX &orthogonality)
    -> bool {

  UINT8 *data_begin = nullptr;
  if (FAILED(constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, view);
  XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, orthogonality);
  memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
  constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto ScreenQuadMaterial::GetConstantBuffer() const -> ResourceSharedPtr {
  return constant_buffer_;
}

void ScreenQuadMaterial::SetExternalConstantBuffer(
    const ResourceSharedPtr &constant_buffer,
    const XMMATRIX &initial_world,
    const XMMATRIX &initial_view,
    const XMMATRIX &initial_ortho) {
  constant_buffer_ = constant_buffer;
  XMStoreFloat4x4(&matrix_constant_data_.world_, initial_world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, initial_view);
  XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, initial_ortho);
}

auto ScreenQuadMaterial::InitializeRootSignature() -> bool {

  CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
  descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[2];
  root_parameters[0].InitAsDescriptorTable(1, &descriptor_ranges_srv[0],
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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

auto ScreenQuadMaterial::InitializeGraphicsPipelineState() -> bool {

  D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

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

  auto device = device_->GetD3d12Device();

  PipelineStateObjectPtr pso = {};
  if (FAILED(
          device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }
  SetPSOByName("bitmap_normal", pso);

  PipelineStateObjectPtr pso2 = {};
  pso_desc.DepthStencilState.DepthEnable = false;
  if (FAILED(device->CreateGraphicsPipelineState(&pso_desc,
                                                 IID_PPV_ARGS(&pso2)))) {
    return false;
  }
  SetPSOByName("bitmap_depth_disable", pso2);

  return true;
}
