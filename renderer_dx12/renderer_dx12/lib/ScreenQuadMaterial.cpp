#include "stdafx.h"

#include "ScreenQuadMaterial.h"

#include "DirectX12Device.h"
#include "PipelineStateBuilder.h"
#include "RootSignatureBuilder.h"

using namespace DirectX;

auto ScreenQuadMaterial::Initialize() -> bool {

  if (initialized_) {
    return true;
  }

  if (!device_) {
    return false;
  }

  if (!external_constant_buffer_) {
    if (!constant_buffer_.Initialize(device_)) {
      return false;
    }
    current_constant_buffer_ = constant_buffer_.GetResource();
  } else {
    current_constant_buffer_ = external_constant_buffer_;
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

  XMStoreFloat4x4(&matrix_constant_data_.world_, world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, view);
  XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, orthogonality);

  if (external_constant_buffer_) {
    UINT8 *data_begin = nullptr;
    if (FAILED(external_constant_buffer_->Map(
            0, nullptr, reinterpret_cast<void **>(&data_begin)))) {
      return false;
    }
    memcpy(data_begin, &matrix_constant_data_, sizeof(MatrixBufferType));
    external_constant_buffer_->Unmap(0, nullptr);
    return true;
  }

  return constant_buffer_.Update(matrix_constant_data_);
}

auto ScreenQuadMaterial::GetConstantBuffer() const -> ResourceSharedPtr {
  return current_constant_buffer_;
}

void ScreenQuadMaterial::SetExternalConstantBuffer(
    const ResourceSharedPtr &constant_buffer,
    const XMMATRIX &initial_world,
    const XMMATRIX &initial_view,
    const XMMATRIX &initial_ortho) {
  external_constant_buffer_ = constant_buffer;
  current_constant_buffer_ = constant_buffer;
  XMStoreFloat4x4(&matrix_constant_data_.world_, initial_world);
  XMStoreFloat4x4(&matrix_constant_data_.view_, initial_view);
  XMStoreFloat4x4(&matrix_constant_data_.orthogonality_, initial_ortho);
}

auto ScreenQuadMaterial::InitializeRootSignature() -> bool {

  RootSignatureBuilder builder;
  builder.AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                             D3D12_SHADER_VISIBILITY_PIXEL);
  builder.AddConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

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

auto ScreenQuadMaterial::InitializeGraphicsPipelineState() -> bool {

  if (!device_) {
    return false;
  }

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

  PipelineStateObjectPtr pso = nullptr;
  if (!builder.Build(device_, pso)) {
    return false;
  }
  SetPSOByName("bitmap_normal", pso);

  auto depth_disabled_desc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  depth_disabled_desc.DepthEnable = FALSE;
  builder.SetDepthStencilState(depth_disabled_desc);

  PipelineStateObjectPtr pso2 = nullptr;
  if (!builder.Build(device_, pso2)) {
    return false;
  }
  SetPSOByName("bitmap_depth_disable", pso2);

  return true;
}
