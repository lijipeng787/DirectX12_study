#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

#include "TypeDefine.h"

#include <unordered_map>
#include <wrl.h>

typedef Microsoft::WRL::ComPtr<ID3DBlob> BlobPtr;

typedef std::vector<BlobPtr> VSBlobVector;

typedef std::vector<BlobPtr> PSBlobVector;

typedef std::unordered_map<std::string, unsigned int> VSIndexContainer;

typedef std::unordered_map<std::string, unsigned int> PSIndexContainer;

namespace ResourceLoader {

class ShaderLoader {
public:
  ShaderLoader() {}

  ShaderLoader(const ShaderLoader &rhs) = delete;

  ShaderLoader &operator==(const ShaderLoader &rhs) = delete;

  ~ShaderLoader() {}

public:
  bool CreateVSAndPSFromFile(WCHAR *vs_shader_filename,
                             WCHAR *ps_shader_filename);

  bool CreateVSFromFile(WCHAR *vs_shader_filename);

  bool CreatePSFromFile(WCHAR *ps_shader_filename);

  void SetVSEntryPoint(LPCSTR entry_point);

  void SetPSEntryPoint(LPCSTR entry_point);

  void SetVSTargetVersion(LPCSTR target_version);

  void SetPSTargetVersion(LPCSTR target_version);

  BlobPtr GetVertexShaderBlobByIndex(unsigned int index) const;

  BlobPtr GetPixelShaderBlobByIndex(unsigned int index) const;

  BlobPtr GetVertexShaderBlobByFileName(WCHAR *filename) const;

  BlobPtr GetPixelShaderBlobByFileName(WCHAR *filename) const;

  BlobPtr GetVertexShaderBlobByEntryName(WCHAR *entry_name) const;

  BlobPtr GetPixelShaderBlobByEntryName(WCHAR *entry_name) const;

private:
  VSIndexContainer vs_index_container_ = {};

  PSIndexContainer ps_index_container_ = {};

  VSBlobVector vertex_shader_container = {};

  PSBlobVector pixel_shader_container = {};

  BlobPtr last_compile_error_ = nullptr;

  LPCSTR vs_entry_point_ = {};

  LPCSTR ps_entry_point_ = {};

  LPCSTR vs_target_version_ = "vs_5_0";

  LPCSTR ps_target_version_ = "ps_5_0";
};

} // namespace ResourceLoader
#endif