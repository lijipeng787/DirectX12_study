#include "stdafx.h"

#include "RootSignatureBuilder.h"

RootSignatureBuilder &RootSignatureBuilder::AddDescriptorTable(
    D3D12_DESCRIPTOR_RANGE_TYPE type, UINT num_descriptors,
    UINT base_shader_register, UINT register_space,
    D3D12_SHADER_VISIBILITY visibility) {
  ParameterDesc desc = {};
  desc.type = ParameterType::DescriptorTable;
  desc.visibility = visibility;
  desc.table.visibility = visibility;
  desc.table.ranges.emplace_back(type, num_descriptors, base_shader_register,
                                 register_space);
  parameters_.push_back(std::move(desc));
  return *this;
}

RootSignatureBuilder &RootSignatureBuilder::AddConstantBufferView(
    UINT shader_register, UINT register_space,
    D3D12_SHADER_VISIBILITY visibility) {
  ParameterDesc desc = {};
  desc.type = ParameterType::ConstantBufferView;
  desc.shader_register = shader_register;
  desc.register_space = register_space;
  desc.visibility = visibility;
  parameters_.push_back(std::move(desc));
  return *this;
}

RootSignatureBuilder &RootSignatureBuilder::AddStaticSampler(
    const D3D12_STATIC_SAMPLER_DESC &sampler_desc) {
  static_samplers_.push_back(sampler_desc);
  return *this;
}

bool RootSignatureBuilder::Build(
    const std::shared_ptr<DirectX12Device> &device,
    RootSignaturePtr &root_signature,
    D3D12_ROOT_SIGNATURE_FLAGS flags) const {
  if (!device) {
    return false;
  }

  auto d3d_device = device->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  std::vector<CD3DX12_ROOT_PARAMETER> root_parameters;
  root_parameters.reserve(parameters_.size());

  UINT total_range_count = 0;
  for (const auto &parameter : parameters_) {
    if (parameter.type == ParameterType::DescriptorTable) {
      total_range_count +=
          static_cast<UINT>(parameter.table.ranges.size());
    }
  }

  std::vector<CD3DX12_DESCRIPTOR_RANGE> flattened_ranges;
  flattened_ranges.reserve(total_range_count);
  for (const auto &parameter : parameters_) {
    if (parameter.type == ParameterType::DescriptorTable) {
      flattened_ranges.insert(flattened_ranges.end(),
                              parameter.table.ranges.begin(),
                              parameter.table.ranges.end());
    }
  }

  UINT range_index = 0;
  for (const auto &parameter : parameters_) {
    if (parameter.type == ParameterType::DescriptorTable) {
      auto range_count = static_cast<UINT>(parameter.table.ranges.size());
      CD3DX12_ROOT_PARAMETER root_param = {};
      root_param.InitAsDescriptorTable(
          range_count, range_count > 0 ? &flattened_ranges[range_index] : nullptr,
          parameter.visibility);
      root_parameters.push_back(root_param);
      range_index += range_count;
    } else {
      CD3DX12_ROOT_PARAMETER root_param = {};
      root_param.InitAsConstantBufferView(parameter.shader_register,
                                          parameter.register_space,
                                          parameter.visibility);
      root_parameters.push_back(root_param);
    }
  }

  CD3DX12_ROOT_SIGNATURE_DESC root_desc(
      static_cast<UINT>(root_parameters.size()), root_parameters.data(),
      static_cast<UINT>(static_samplers_.size()), static_samplers_.data(),
      flags);

  Microsoft::WRL::ComPtr<ID3DBlob> signature_blob = nullptr;
  Microsoft::WRL::ComPtr<ID3DBlob> error_blob = nullptr;
  auto hr = D3D12SerializeRootSignature(
      &root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);
  if (FAILED(hr)) {
    return false;
  }

  hr = d3d_device->CreateRootSignature(
      0, signature_blob->GetBufferPointer(),
      signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature));
  if (FAILED(hr)) {
    return false;
  }

  return true;
}


