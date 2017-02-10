#pragma once

#ifndef _MATERIAL_H_
#define _MATERIAL_H_

#include <d3d12.h>

#include "TypeDefine.h"

namespace Effect {

	class Material {
	public:
		Material();

		Material(const Material& rhs) = delete;

		Material& operator=(const Material& rhs) = delete;

		virtual ~Material();
	public:
		//virtual bool Initialize() = 0;

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

		PipelineStateObjectPtr GetPSO()const;

		void SetPSO(const PipelineStateObjectPtr& pso);

		PipelineStateObjectPtr GetSecondPSO()const;

		void SetSecondPSO(const PipelineStateObjectPtr& pso);

		PipelineStateObjectPtr GetThirdPSO()const;

		void SetThirdPSO(const PipelineStateObjectPtr& pso);
	private:
		VertexShaderByteCode vertex_shader_bitecode_ = {};

		PixelShaderByteCode pixel_shader_bitcode_ = {};

		RootSignaturePtr root_signature_ = nullptr;

		PipelineStateObjectPtr pso_ = nullptr;

		PipelineStateObjectPtr pso_depth_disabled_ = nullptr;

		PipelineStateObjectPtr pso_blend_enable_ = nullptr;
	};

}
#endif // !_MATERIAL_H_
