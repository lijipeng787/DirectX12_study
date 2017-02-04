#ifndef _FONTSHADERCLASS_H_
#define _FONTSHADERCLASS_H_

#include <DirectXMath.h>
#include <d3dcompiler.h>
#include "DirectX12Device.h"

class FontShader{
public:
	FontShader() {}
	
	FontShader(const FontShader& rhs) = delete;

	FontShader& operator==(const FontShader& rhs) = delete;
	
	~FontShader() {}
public:
	bool Initialize(WCHAR *vs_shader_filename, WCHAR *ps_shader_filename);

	const BlobPtr& GetVertexShaderBlob()const { return vertex_shader_blob_; }

	const BlobPtr& GetPixelSHaderBlob()const { return pixel_shader_blob_; }
private:
	BlobPtr vertex_shader_blob_ = nullptr;

	BlobPtr pixel_shader_blob_ = nullptr;

	BlobPtr shader_error_blob = nullptr;
};

#endif