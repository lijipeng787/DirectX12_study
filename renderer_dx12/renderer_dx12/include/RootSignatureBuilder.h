#pragma once

#include <memory>
#include <vector>

#include "DirectX12Device.h"
#include "d3dx12.h"
#include "TypeDefine.h"

class RootSignatureBuilder {
public:
  RootSignatureBuilder() = default;

  RootSignatureBuilder(const RootSignatureBuilder &) = delete;
  
  RootSignatureBuilder &operator=(const RootSignatureBuilder &) = delete;

  RootSignatureBuilder(RootSignatureBuilder &&) noexcept = default;
  
  RootSignatureBuilder &operator=(RootSignatureBuilder &&) noexcept = default;

  ~RootSignatureBuilder() = default;

  RootSignatureBuilder &AddDescriptorTable(
      D3D12_DESCRIPTOR_RANGE_TYPE type, UINT num_descriptors,
      UINT base_shader_register, UINT register_space,
      D3D12_SHADER_VISIBILITY visibility);

  RootSignatureBuilder &AddConstantBufferView(
      UINT shader_register, UINT register_space,
      D3D12_SHADER_VISIBILITY visibility);

  RootSignatureBuilder &
  AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC &sampler_desc);

  bool Build(const std::shared_ptr<DirectX12Device> &device,
             RootSignaturePtr &root_signature,
             D3D12_ROOT_SIGNATURE_FLAGS flags) const;

private:
  struct DescriptorTableDesc {
    std::vector<CD3DX12_DESCRIPTOR_RANGE> ranges = {};
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
  };

  enum class ParameterType { DescriptorTable, ConstantBufferView };

  struct ParameterDesc {
    ParameterType type = ParameterType::ConstantBufferView;
    DescriptorTableDesc table = {};
    UINT shader_register = 0;
    UINT register_space = 0;
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;
  };

  std::vector<ParameterDesc> parameters_ = {};
  
  std::vector<D3D12_STATIC_SAMPLER_DESC> static_samplers_ = {};
};


