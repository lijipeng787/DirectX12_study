#include "stdafx.h"

#include "TextMaterial.h"

#include "DirectX12Device.h"

using namespace DirectX;

TextMaterial::~TextMaterial() = default;

auto TextMaterial::Initialize() -> bool {

  if (!InitializeRootSignature()) {
    return false;
  }

  if (!InitializeGraphicsPipelineState()) {
    return false;
  }

  if (!InitializeConstantBuffer()) {
    return false;
  }

  return true;
}

auto TextMaterial::InitializeRootSignature() -> bool {

  CD3DX12_DESCRIPTOR_RANGE descriptor_ranges_srv[1];
  descriptor_ranges_srv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

  CD3DX12_ROOT_PARAMETER root_parameters[3];
  root_parameters[0].InitAsDescriptorTable(1, &descriptor_ranges_srv[0],
                                           D3D12_SHADER_VISIBILITY_PIXEL);
  root_parameters[1].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  root_parameters[2].InitAsConstantBufferView(
      0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

auto TextMaterial::InitializeGraphicsPipelineState() -> bool {

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

  PipelineStateObjectPtr pso = {};
  if (FAILED(device_->GetD3d12Device()->CreateGraphicsPipelineState(
          &pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }
  SetPSOByName("text_normal", pso);

  D3D12_BLEND_DESC blend_desc = {};
  blend_desc.AlphaToCoverageEnable = false;
  blend_desc.IndependentBlendEnable = false;
  blend_desc.RenderTarget[0].BlendEnable = true;
  blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
  blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
  blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
  blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = 0x0f;

  pso_desc.BlendState = blend_desc;

  pso.Reset();
  if (FAILED(device_->GetD3d12Device()->CreateGraphicsPipelineState(
          &pso_desc, IID_PPV_ARGS(&pso)))) {
    return false;
  }
  SetPSOByName("text_blend_enable", pso);

  return true;
}

auto TextMaterial::InitializeConstantBuffer() -> bool {

  if (FAILED(device_->GetD3d12Device()->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&matrix_constant_buffer_)))) {
    return false;
  }

  if (FAILED(device_->GetD3d12Device()->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE,
          &CD3DX12_RESOURCE_DESC::Buffer(sizeof(PixelBufferType)),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&pixel_color_constant_buffer_)))) {
    return false;
  }

  return true;
}

auto TextMaterial::UpdateMatrixConstant(const XMMATRIX &world,
                                        const XMMATRIX &base_view,
                                        const XMMATRIX &orthonality) -> bool {

  UINT8 *data_begin = nullptr;
  if (FAILED(matrix_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.base_view_, base_view);
  XMStoreFloat4x4(&matrix_constant_data_.orthonality_, orthonality);
  memcpy(data_begin, &matrix_constant_data_, sizeof(ConstantBufferType));
  matrix_constant_buffer_->Unmap(0, nullptr);

  return true;
}

auto TextMaterial::UpdateLightConstant(const XMFLOAT4 &pixel_color) -> bool {

  UINT8 *data_begin = nullptr;
  if (FAILED(pixel_color_constant_buffer_->Map(
          0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
    return false;
  }

  pixel_color_constant_data_.pixel_color_ = pixel_color;
  memcpy(data_begin, &pixel_color_constant_data_, sizeof(PixelBufferType));
  pixel_color_constant_buffer_->Unmap(0, nullptr);

  return true;
}


