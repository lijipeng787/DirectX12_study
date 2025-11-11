#include "stdafx.h"

#include "ShaderLoader.h"

#include <d3dcompiler.h>
#include <windows.h>

#include <sstream>

namespace {

std::string ToAnsiString(const std::wstring &source) {
  std::string result;
  if (!source.empty()) {
    ResourceLoader::WCHARToString(const_cast<WCHAR *>(source.c_str()), result);
  }
  return result;
}

const char *ShaderTypeName(bool is_vertex_shader) {
  return is_vertex_shader ? "vertex" : "pixel";
}

void OutputErrorMessage(const std::string &message) {
  if (!message.empty()) {
    OutputDebugStringA(message.c_str());
  }
}

} // namespace

namespace ResourceLoader {

bool ShaderLoader::CompileVertexAndPixelShaders(
    const ShaderCompileDesc &vs_desc, const ShaderCompileDesc &ps_desc) {
  if (!CompileVertexShader(vs_desc)) {
    return false;
  }
  if (!CompilePixelShader(ps_desc)) {
    return false;
  }
  return true;
}

bool ShaderLoader::CompileVertexShader(const ShaderCompileDesc &desc) {
  return CompileShaderInternal(desc, true);
}

bool ShaderLoader::CompilePixelShader(const ShaderCompileDesc &desc) {
  return CompileShaderInternal(desc, false);
}

BlobPtr ShaderLoader::GetVertexShaderBlobByIndex(unsigned int index) const {
  return vertex_shader_container.at(index);
}

BlobPtr ShaderLoader::GetPixelShaderBlobByIndex(unsigned int index) const {
  return pixel_shader_container.at(index);
}

BlobPtr ShaderLoader::GetVertexShaderBlob(const ShaderCompileDesc &desc) const {
  const std::string key = BuildShaderKey(desc);
  const auto it = vs_index_container_.find(key);
  if (it == vs_index_container_.end()) {
    return nullptr;
  }
  return GetVertexShaderBlobByIndex(it->second);
}

BlobPtr ShaderLoader::GetPixelShaderBlob(const ShaderCompileDesc &desc) const {
  const std::string key = BuildShaderKey(desc);
  const auto it = ps_index_container_.find(key);
  if (it == ps_index_container_.end()) {
    return nullptr;
  }
  return GetPixelShaderBlobByIndex(it->second);
}

BlobPtr ShaderLoader::GetVertexShaderBlobByFileName(WCHAR *filename) const {
  std::string s;
  WCHARToString(filename, s);
  const auto it = vs_file_index_container_.find(s);
  if (it == vs_file_index_container_.end()) {
    return nullptr;
  }
  return GetVertexShaderBlobByIndex(it->second);
}

BlobPtr ShaderLoader::GetPixelShaderBlobByFileName(WCHAR *filename) const {
  std::string s;
  WCHARToString(filename, s);
  const auto it = ps_file_index_container_.find(s);
  if (it == ps_file_index_container_.end()) {
    return nullptr;
  }
  return GetPixelShaderBlobByIndex(it->second);
}

BlobPtr ShaderLoader::GetVertexShaderBlobByEntryName(WCHAR * /*entry_name*/) const {
  return BlobPtr();
}

BlobPtr ShaderLoader::GetPixelShaderBlobByEntryName(WCHAR * /*entry_name*/) const {
  return BlobPtr();
}

const std::string &ShaderLoader::GetLastErrorMessage() const {
  return last_error_message_;
}

bool ShaderLoader::ValidateCompileDesc(const ShaderCompileDesc &desc,
                                       bool is_vertex_shader) {
  if (desc.file_path.empty()) {
    std::ostringstream oss;
    oss << "ShaderLoader: " << ShaderTypeName(is_vertex_shader)
        << " shader file path is empty.";
    last_error_message_ = oss.str();
    OutputErrorMessage(last_error_message_);
    return false;
  }

  if (desc.entry_point.empty()) {
    std::ostringstream oss;
    oss << "ShaderLoader: entry point is empty for "
        << ShaderTypeName(is_vertex_shader) << " shader.";
    last_error_message_ = oss.str();
    OutputErrorMessage(last_error_message_);
    return false;
  }

  if (desc.target.empty()) {
    std::ostringstream oss;
    oss << "ShaderLoader: target profile is empty for "
        << ShaderTypeName(is_vertex_shader) << " shader '"
        << desc.entry_point << "'.";
    last_error_message_ = oss.str();
    OutputErrorMessage(last_error_message_);
    return false;
  }

  return true;
}

bool ShaderLoader::CompileShaderInternal(const ShaderCompileDesc &desc,
                                         bool is_vertex_shader) {
  last_error_message_.clear();

  if (!ValidateCompileDesc(desc, is_vertex_shader)) {
    return false;
  }

  const std::string file_key = ToAnsiString(desc.file_path);
  const std::string shader_key = BuildShaderKey(desc);

  ShaderIndexContainer &index_container =
      is_vertex_shader ? vs_index_container_ : ps_index_container_;
  ShaderIndexContainer &file_index_container =
      is_vertex_shader ? vs_file_index_container_ : ps_file_index_container_;
  auto &blob_container = is_vertex_shader ? vertex_shader_container
                                          : pixel_shader_container;

  if (index_container.find(shader_key) != index_container.end()) {
    return true;
  }

#if defined(_DEBUG)
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  UINT compile_flags = 0;
#endif

  BlobPtr shader_blob;
  HRESULT hr = D3DCompileFromFile(
      desc.file_path.c_str(), desc.defines, nullptr, desc.entry_point.c_str(),
      desc.target.c_str(), compile_flags, 0, &shader_blob, &last_compile_error_);

  if (FAILED(hr)) {
    if (last_compile_error_) {
      last_error_message_.assign(
          static_cast<const char *>(last_compile_error_->GetBufferPointer()),
          static_cast<size_t>(last_compile_error_->GetBufferSize()));
      last_compile_error_.Reset();
    } else {
      std::ostringstream oss;
      oss << "ShaderLoader: Failed to compile " << ShaderTypeName(is_vertex_shader)
          << " shader '" << desc.entry_point << "' from file '" << file_key
          << "'. HRESULT=0x" << std::hex << hr;
      last_error_message_ = oss.str();
    }
    OutputErrorMessage(last_error_message_);
    return false;
  }

  blob_container.push_back(shader_blob);
  const unsigned int index =
      static_cast<unsigned int>(blob_container.size() - 1);

  index_container.emplace(shader_key, index);
  if (file_index_container.find(file_key) == file_index_container.end()) {
    file_index_container.emplace(file_key, index);
  }

  last_error_message_.clear();
  return true;
}

std::string ShaderLoader::BuildShaderKey(const ShaderCompileDesc &desc) const {
  std::string key = ToAnsiString(desc.file_path);
  key += '|';
  key += desc.entry_point;
  key += '|';
  key += desc.target;

  if (desc.defines != nullptr) {
    const D3D_SHADER_MACRO *macro = desc.defines;
    while (macro != nullptr && macro->Name != nullptr) {
      key += '|';
      key += macro->Name;
      key += '=';
      if (macro->Definition != nullptr) {
        key += macro->Definition;
      }
      ++macro;
    }
  }

  return key;
}

} // namespace ResourceLoader
