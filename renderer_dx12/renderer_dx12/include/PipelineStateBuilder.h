#pragma once

#include <memory>

#include "DirectX12Device.h"
#include "TypeDefine.h"
#include "d3dx12.h"

class GraphicsPipelineStateBuilder {
public:
  GraphicsPipelineStateBuilder();

  GraphicsPipelineStateBuilder(const GraphicsPipelineStateBuilder &) = delete;
  
  auto operator=(const GraphicsPipelineStateBuilder &) -> GraphicsPipelineStateBuilder & = delete;

  GraphicsPipelineStateBuilder(GraphicsPipelineStateBuilder &&) noexcept = default;
  
  auto operator=(GraphicsPipelineStateBuilder &&) noexcept -> GraphicsPipelineStateBuilder & = default;

  ~GraphicsPipelineStateBuilder() = default;

  void SetRootSignature(const RootSignaturePtr &root_signature);

  void SetVertexShader(const VertexShaderByteCode &bytecode);

  void SetPixelShader(const PixelShaderByteCode &bytecode);

  void SetInputLayout(const D3D12_INPUT_ELEMENT_DESC *input_elements,
                      UINT element_count);
  
  void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE topology);
  
  void SetRenderTargetFormats(UINT num_render_targets,
                              const DXGI_FORMAT *rtv_formats,
                              DXGI_FORMAT dsv_format);
  void SetSampleDesc(const DXGI_SAMPLE_DESC &sample_desc);

  void SetRasterizerState(const D3D12_RASTERIZER_DESC &rasterizer_desc);
  
  void SetBlendState(const D3D12_BLEND_DESC &blend_desc);
  
  void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &depth_desc);

  auto Build(const std::shared_ptr<DirectX12Device> &device,
             PipelineStateObjectPtr &pipeline_state) const -> bool;

private:
  D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_ = {};

  RootSignaturePtr root_signature_ref_ = nullptr;
};


