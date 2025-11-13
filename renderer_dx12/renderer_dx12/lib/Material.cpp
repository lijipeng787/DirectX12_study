#include "stdafx.h"

#include "Material.h"

VertexShaderByteCode Effect::Material::GetVSByteCode() const {
  return vertex_shader_bytecode_;
}

PixelShaderByteCode Effect::Material::GetPSByteCode() const {
  return pixel_shader_bytecode_;
}

void Effect::Material::SetVSByteCode(const VertexShaderByteCode &bytecode) {
  vertex_shader_bytecode_ = bytecode;
}

void Effect::Material::SetPSByteCode(const PixelShaderByteCode &bytecode) {
  pixel_shader_bytecode_ = bytecode;
}

RootSignaturePtr Effect::Material::GetRootSignature() const {
  return root_signature_;
}

void Effect::Material::SetRootSignature(
    const RootSignaturePtr &root_signature) {
  root_signature_ = root_signature;
}

PipelineStateObjectPtr
Effect::Material::GetPSOByIndex(unsigned int index) const {
  return pso_container_.at(index);
}

PipelineStateObjectPtr Effect::Material::GetPSOByName(std::string name) const {
  const auto it = pso_index_container_.find(name);
  if (it == pso_index_container_.end()) {
    return nullptr;
  }
  return GetPSOByIndex(it->second);
}

void Effect::Material::SetPSOByName(std::string name,
                                    const PipelineStateObjectPtr &pso) {

  if (pso_index_container_.find(name) != pso_index_container_.end()) {
    return;
  }
  pso_container_.push_back(pso);
  unsigned int index = static_cast<unsigned int>(pso_container_.size() - 1);
  pso_index_container_.insert(std::make_pair(name, index));
}
