#include "fontshaderclass.h"

bool FontShader::Initialize(WCHAR * vs_shader_filename, WCHAR * ps_shader_filename) {

#if defined(_DEBUG)
	UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flags = 0;
#endif

	if (FAILED(D3DCompileFromFile(
		vs_shader_filename,
		nullptr,
		nullptr,
		"FontVertexShader",
		"vs_5_0",
		compile_flags,
		0,
		&vertex_shader_blob_,
		&shader_error_blob
		))) {
		if (nullptr != shader_error_blob) {
			char err[256] = {};
			memcpy(err, shader_error_blob.Get()->GetBufferPointer(), shader_error_blob.Get()->GetBufferSize());
			assert(0);
		}
		return false;
	}

	shader_error_blob = nullptr;
	if (FAILED(D3DCompileFromFile(
		ps_shader_filename,
		nullptr,
		nullptr,
		"FontPixelShader",
		"ps_5_0",
		compile_flags,
		0,
		&pixel_shader_blob_,
		&shader_error_blob
		))) {
		if (nullptr != shader_error_blob) {
			char err[256] = {};
			memcpy(err, shader_error_blob.Get()->GetBufferPointer(), shader_error_blob.Get()->GetBufferSize());
			assert(0);
		}
		return false;
	}

	return true;

}
