#include "stdafx.h"

#include "TextMaterial.h"

#include "DirectX12Device.h"
#include "PipelineStateBuilder.h"
#include "RootSignatureBuilder.h"

using namespace DirectX;

TextMaterial::~TextMaterial() = default;

auto TextMaterial::Initialize() -> bool {

  if (!device_) {
    return false;
  }

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

  RootSignatureBuilder builder;
  builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                             D3D12_SHADER_VISIBILITY_PIXEL);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

  builder.AddStaticSampler(sampler_desc);

  RootSignaturePtr root_signature = {};
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

auto TextMaterial::InitializeGraphicsPipelineState() -> bool {

  D3D12_INPUT_ELEMENT_DESC input_element_descs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

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

  PipelineStateObjectPtr pso = {};
  if (!builder.Build(device_, pso)) {
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

  builder.SetBlendState(blend_desc);

  pso.Reset();
  if (!builder.Build(device_, pso)) {
    return false;
  }
  SetPSOByName("text_blend_enable", pso);

  return true;
}

auto TextMaterial::InitializeConstantBuffer() -> bool {

  if (!matrix_constant_buffer_.Initialize(device_)) {
    return false;
  }

  if (!pixel_color_constant_buffer_.Initialize(device_)) {
    return false;
  }

  return true;
}

auto TextMaterial::UpdateMatrixConstant(const XMMATRIX &world,
                                        const XMMATRIX &base_view,
                                        const XMMATRIX &orthonality) -> bool {

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.base_view_, base_view);
  XMStoreFloat4x4(&matrix_constant_data_.orthonality_, orthonality);

  return matrix_constant_buffer_.Update(matrix_constant_data_);
}

auto TextMaterial::UpdateLightConstant(const XMFLOAT4 &pixel_color) -> bool {

  pixel_color_constant_data_.pixel_color_ = pixel_color;

  return pixel_color_constant_buffer_.Update(pixel_color_constant_data_);
}


