#include "stdafx.h"
#include "Material.h"

VertexShaderByteCode Effect::Material::GetVSByteCode() const{
	return vertex_shader_bitecode_;
}

PixelShaderByteCode Effect::Material::GetPSByteCode() const{
	return pixel_shader_bitcode_;
}

void Effect::Material::SetVSByteCode(const VertexShaderByteCode & bytecode){
	vertex_shader_bitecode_ = bytecode;
}

void Effect::Material::SetPSByteCode(const PixelShaderByteCode & bytecode){
	pixel_shader_bitcode_ = bytecode;
}

RootSignaturePtr Effect::Material::GetRootSignature() const{
	return root_signature_;
}

void Effect::Material::SetRootSignature(const RootSignaturePtr & root_signature){
	root_signature_.Swap(root_signature.Get());
}

PipelineStateObjectPtr Effect::Material::GetPSO() const{
	return pso_;
}

void Effect::Material::SetPSO(const PipelineStateObjectPtr & pso){
	pso_.Swap(pso.Get());
}

PipelineStateObjectPtr Effect::Material::GetSecondPSO() const{
	return pso_depth_disabled_;
}

void Effect::Material::SetSecondPSO(const PipelineStateObjectPtr & pso){
	pso_depth_disabled_.Swap(pso.Get());
}

PipelineStateObjectPtr Effect::Material::GetThirdPSO() const{
	return pso_blend_enable_;
}

void Effect::Material::SetThirdPSO(const PipelineStateObjectPtr & pso){
	pso_blend_enable_.Swap(pso.Get());
}

Effect::Material::Material(){
}

Effect::Material::~Material(){
}
