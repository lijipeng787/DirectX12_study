#include "stdafx.h"

#include "Material.h"

using namespace std;

VertexShaderByteCode Effect::Material::GetVSByteCode() const {
  return vertex_shader_bitecode_;
}

PixelShaderByteCode Effect::Material::GetPSByteCode() const {
  return pixel_shader_bitcode_;
}

void Effect::Material::SetVSByteCode(const VertexShaderByteCode &bytecode) {
  vertex_shader_bitecode_ = bytecode;
}

void Effect::Material::SetPSByteCode(const PixelShaderByteCode &bytecode) {
  pixel_shader_bitcode_ = bytecode;
}

RootSignaturePtr Effect::Material::GetRootSignature() const {
  return root_signature_;
}

void Effect::Material::SetRootSignature(
    const RootSignaturePtr &root_signature) {
  root_signature_.Swap(root_signature.Get());
}

PipelineStateObjectPtr
Effect::Material::GetPSOByIndex(unsigned int index) const {
  return pso_cotainer_.at(index);
}

PipelineStateObjectPtr Effect::Material::GetPSOByName(std::string name) const {
  unsigned int index = pso_index_cotainer_.find(name)->second;
  return GetPSOByIndex(index);
}

void Effect::Material::SetPSOByName(std::string name,
                                    const PipelineStateObjectPtr &pso) {

  if (pso_index_cotainer_.find(name) != pso_index_cotainer_.end()) {
    return;
  }
  pso_cotainer_.push_back(pso);
  unsigned int index = pso_cotainer_.size() - 1;
  pso_index_cotainer_.insert(make_pair(name, index));
}
