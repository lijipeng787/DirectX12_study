#pragma once

#ifndef _TEXTURESHADERCLASS_H_
#define _TEXTURESHADERCLASS_H_

#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "../d3d12 common/DirectX12Device.h"

class BitMapShaderLoader{

public:
	BitMapShaderLoader() {}

	BitMapShaderLoader(const BitMapShaderLoader& rhs) = delete;

	BitMapShaderLoader& operator=(const BitMapShaderLoader& rhs) = delete;

	~BitMapShaderLoader() {}

public:
	bool Initialize(WCHAR *vs_shader_filename, WCHAR *ps_shader_filename);

	BlobPtr GetVertexShaderBlob()const { return vertex_shader_blob_; }

	BlobPtr GetPixelSHaderBlob()const { return pixel_shader_blob_; }

private:
	BlobPtr vertex_shader_blob_ = nullptr;

	BlobPtr pixel_shader_blob_ = nullptr;

	BlobPtr shader_error_blob = nullptr;
};

#endif // !_TEXTURESHADERCLASS_H_
