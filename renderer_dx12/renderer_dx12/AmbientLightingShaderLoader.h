#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

#include <DirectXMath.h>
#include <d3dcompiler.h>

#include "../d3d12 common/DirectX12Device.h"

class AmbientLightingShaderLoader{

public:
	AmbientLightingShaderLoader() {}

	AmbientLightingShaderLoader(const AmbientLightingShaderLoader& rhs) = delete;

	AmbientLightingShaderLoader& operator==(const AmbientLightingShaderLoader& rhs) = delete;
	
	~AmbientLightingShaderLoader() {}

public:
	bool Initialize(WCHAR *vs_shader_filename, WCHAR *ps_shader_filename);

	BlobPtr AmbientLightingShaderLoader::GetVertexShaderBlob() const { return vertex_shader_blob_; }

	BlobPtr AmbientLightingShaderLoader::GetPixelSHaderBlob() const { return pixel_shader_blob_; }

private:
	BlobPtr vertex_shader_blob_ = nullptr;

	BlobPtr pixel_shader_blob_ = nullptr;

	BlobPtr shader_error_blob = nullptr;
};

#endif