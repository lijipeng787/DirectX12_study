#include "stdafx.h"

#include "PipelineStateBuilder.h"

GraphicsPipelineStateBuilder::GraphicsPipelineStateBuilder() {
  desc_.SampleMask = UINT_MAX;
  desc_.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  desc_.SampleDesc.Count = 1;
  desc_.SampleDesc.Quality = 0;
  desc_.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  desc_.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  desc_.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  desc_.NumRenderTargets = 1;
  desc_.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc_.DSVFormat = DXGI_FORMAT_D32_FLOAT;
}

void GraphicsPipelineStateBuilder::SetRootSignature(
    const RootSignaturePtr &root_signature) {
  root_signature_ref_ = root_signature;
  desc_.pRootSignature =
      root_signature_ref_ ? root_signature_ref_.Get() : nullptr;
}

void GraphicsPipelineStateBuilder::SetVertexShader(
    const VertexShaderByteCode &bytecode) {
  desc_.VS = bytecode;
}

void GraphicsPipelineStateBuilder::SetPixelShader(
    const PixelShaderByteCode &bytecode) {
  desc_.PS = bytecode;
}

void GraphicsPipelineStateBuilder::SetInputLayout(
    const D3D12_INPUT_ELEMENT_DESC *input_elements, UINT element_count) {
  desc_.InputLayout = {input_elements, element_count};
}

void GraphicsPipelineStateBuilder::SetPrimitiveTopologyType(
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology) {
  desc_.PrimitiveTopologyType = topology;
}

void GraphicsPipelineStateBuilder::SetRenderTargetFormats(
    UINT num_render_targets, const DXGI_FORMAT *rtv_formats,
    DXGI_FORMAT dsv_format) {
  desc_.NumRenderTargets = num_render_targets;
  for (UINT i = 0; i < num_render_targets && i < _countof(desc_.RTVFormats);
       ++i) {
    desc_.RTVFormats[i] = rtv_formats[i];
  }
  desc_.DSVFormat = dsv_format;
}

void GraphicsPipelineStateBuilder::SetSampleDesc(
    const DXGI_SAMPLE_DESC &sample_desc) {
  desc_.SampleDesc = sample_desc;
}

void GraphicsPipelineStateBuilder::SetRasterizerState(
    const D3D12_RASTERIZER_DESC &rasterizer_desc) {
  desc_.RasterizerState = rasterizer_desc;
}

void GraphicsPipelineStateBuilder::SetBlendState(
    const D3D12_BLEND_DESC &blend_desc) {
  desc_.BlendState = blend_desc;
}

void GraphicsPipelineStateBuilder::SetDepthStencilState(
    const D3D12_DEPTH_STENCIL_DESC &depth_desc) {
  desc_.DepthStencilState = depth_desc;
}

bool GraphicsPipelineStateBuilder::Build(
    const std::shared_ptr<DirectX12Device> &device,
    PipelineStateObjectPtr &pipeline_state) const {
  if (!device) {
    return false;
  }

  auto d3d_device = device->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  auto hr = d3d_device->CreateGraphicsPipelineState(
      &desc_, IID_PPV_ARGS(&pipeline_state));
  return SUCCEEDED(hr);
}


