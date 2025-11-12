#pragma once

#include <unordered_map>

#include "TypeDefine.h"

namespace Effect {

typedef std::vector<PipelineStateObjectPtr> PSOContainer;

typedef std::unordered_map<std::string, unsigned int> PSOIndexContainer;

class Material {
public:
  Material() = default;

  Material(const Material &rhs) = delete;

  auto operator=(const Material &rhs) -> Material & = delete;

  virtual ~Material() = default;

  virtual bool Initialize() = 0;

  // virtual bool Update() = 0;

  // virtual bool PreRender() = 0;

  // virtual void Render() = 0;

  // virtual bool PostRender() = 0;

  VertexShaderByteCode GetVSByteCode() const;

  PixelShaderByteCode GetPSByteCode() const;

  void SetVSByteCode(const VertexShaderByteCode &bytecode);

  void SetPSByteCode(const PixelShaderByteCode &bytecode);

  RootSignaturePtr GetRootSignature() const;

  void SetRootSignature(const RootSignaturePtr &root_signature);

  PipelineStateObjectPtr GetPSOByIndex(unsigned int index) const;

  PipelineStateObjectPtr GetPSOByName(std::string name) const;

  void SetPSOByName(std::string name, const PipelineStateObjectPtr &pso);

private:
  VertexShaderByteCode vertex_shader_bytecode_ = {};

  PixelShaderByteCode pixel_shader_bitcode_ = {};

  RootSignaturePtr root_signature_ = nullptr;

  PSOContainer pso_container_;

  PSOIndexContainer pso_index_container_;
};

} // namespace Effect
