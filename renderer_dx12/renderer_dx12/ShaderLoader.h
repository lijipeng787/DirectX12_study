#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

#include "TypeDefine.h"

#include <unordered_map>
#include <wrl.h>

typedef Microsoft::WRL::ComPtr<ID3DBlob> BlobPtr;

typedef std::vector<BlobPtr> VSBlobVector;

typedef std::vector<BlobPtr> PSBlobVector;

typedef std::unordered_map<std::string, unsigned int> ShaderIndexContainer;

namespace ResourceLoader {

struct ShaderCompileDesc {
  std::wstring file_path;
  std::string entry_point;
  std::string target;
  const D3D_SHADER_MACRO *defines = nullptr;
};

class ShaderLoader {
public:
  ShaderLoader() {}

  ShaderLoader(const ShaderLoader &rhs) = delete;

  ShaderLoader &operator==(const ShaderLoader &rhs) = delete;

  ~ShaderLoader() {}

public:
  bool CompileVertexAndPixelShaders(const ShaderCompileDesc &vs_desc,
                                    const ShaderCompileDesc &ps_desc);

  bool CompileVertexShader(const ShaderCompileDesc &desc);

  bool CompilePixelShader(const ShaderCompileDesc &desc);

  BlobPtr GetVertexShaderBlobByIndex(unsigned int index) const;

  BlobPtr GetPixelShaderBlobByIndex(unsigned int index) const;

  BlobPtr GetVertexShaderBlob(const ShaderCompileDesc &desc) const;

  BlobPtr GetPixelShaderBlob(const ShaderCompileDesc &desc) const;

  BlobPtr GetVertexShaderBlobByFileName(WCHAR *filename) const;

  BlobPtr GetPixelShaderBlobByFileName(WCHAR *filename) const;

  BlobPtr GetVertexShaderBlobByEntryName(WCHAR *entry_name) const;

  BlobPtr GetPixelShaderBlobByEntryName(WCHAR *entry_name) const;

  const std::string &GetLastErrorMessage() const;

private:
  bool ValidateCompileDesc(const ShaderCompileDesc &desc,
                           bool is_vertex_shader);

  bool CompileShaderInternal(const ShaderCompileDesc &desc,
                             bool is_vertex_shader);

  std::string BuildShaderKey(const ShaderCompileDesc &desc) const;

  ShaderIndexContainer vs_index_container_ = {};

  ShaderIndexContainer ps_index_container_ = {};

  ShaderIndexContainer vs_file_index_container_ = {};

  ShaderIndexContainer ps_file_index_container_ = {};

  VSBlobVector vertex_shader_container = {};

  PSBlobVector pixel_shader_container = {};

  BlobPtr last_compile_error_ = nullptr;

  std::string last_error_message_;
};

} // namespace ResourceLoader
#endif