#pragma once

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <unordered_map>
#include <d3d12.h>

#include "TypeDefine.h"

namespace Effect {

	typedef std::vector<PipelineStateObjectPtr> PSOContainer;

	typedef std::unordered_map<std::string, unsigned int> PSOIndexCotainer;

	class Material {
	public:
		Material();

		Material(const Material& rhs) = delete;

		Material& operator=(const Material& rhs) = delete;

		virtual ~Material();
	public:
		virtual bool Initialize() = 0;

		//virtual bool Update() = 0;

		//virtual bool PreRender() = 0;

		//virtual void Render() = 0;

		//virtual bool PostRender() = 0;
	public:
		VertexShaderByteCode GetVSByteCode()const;

		PixelShaderByteCode GetPSByteCode()const;

		void SetVSByteCode(const VertexShaderByteCode& bytecode);

		void SetPSByteCode(const PixelShaderByteCode& bytecode);

		RootSignaturePtr GetRootSignature()const;

		void SetRootSignature(const RootSignaturePtr& root_signature);

		PipelineStateObjectPtr GetPSOByIndex(unsigned int index)const;

		PipelineStateObjectPtr GetPSOByName(std::string name)const;

		void SetPSOByName(std::string name,const PipelineStateObjectPtr& pso);
	private:
		VertexShaderByteCode vertex_shader_bitecode_ = {};

		PixelShaderByteCode pixel_shader_bitcode_ = {};

		RootSignaturePtr root_signature_ = nullptr;

		PSOContainer pso_cotainer_ = {};

		PSOIndexCotainer pso_index_cotainer_ = {};
	};

}
#endif // !_MATERIAL_H_
