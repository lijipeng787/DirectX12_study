#include "stdafx.h"
#include <cassert>

#include "ShaderLoader.h"

using namespace std;

using namespace ResourceLoader;

void WCHARToString(WCHAR* wchar, string& s) {

	wchar_t * wText = wchar;
	DWORD length_of_wchar = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);
	char *tem = new char[length_of_wchar];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, tem, length_of_wchar, NULL, FALSE);
	s = tem;
	delete[]tem;
}

// TODO: finish this function
void StringToWCHAR(string s, WCHAR* wchar) {

	//string temp = s;
	//int length_of_string = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL, 0);
	//wchar_t * wszUtf8 = new wchar_t[length_of_string + 1];
	//memset(wszUtf8, 0, length_of_string * 2 + 2);
	//MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, length_of_string);
	//memcpy(wchar, wszUtf8, length_of_string + 1);
	//delete[] wszUtf8;
}

bool ShaderLoader::CreateVSAndPSFromFile(WCHAR * vs_filename, WCHAR * ps_filename){

	if (CreateVSFromFile(vs_filename) && CreatePSFromFile(ps_filename)) {
		return true;
	}
	return false;
}

bool ShaderLoader::CreateVSFromFile(WCHAR * vs_filename){

#if defined(_DEBUG)
	UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flags = 0;
#endif

	string vs_string = {};
	WCHARToString(vs_filename, vs_string);
	if (vs_index_container_.find(vs_string) != vs_index_container_.end()) {
		return true;
	}

	BlobPtr vertex_shader_blob = {};
	if (FAILED(D3DCompileFromFile(vs_filename, nullptr, nullptr, vs_entry_point_, vs_target_version_,
		compile_flags, 0, &vertex_shader_blob, &last_compile_error_
	))) {
		if (nullptr != last_compile_error_) {
			char err[256] = {};
			memcpy(err, last_compile_error_.Get()->GetBufferPointer(), last_compile_error_.Get()->GetBufferSize());
			assert(0);
		}
		return false;
	}

	vertex_shader_container.push_back(vertex_shader_blob);
	unsigned int index = vertex_shader_container.size() - 1;

	vs_index_container_.insert(make_pair(vs_string, index));

	return true;
}

bool ShaderLoader::CreatePSFromFile(WCHAR * ps_filename){

#if defined(_DEBUG)
	UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compile_flags = 0;
#endif

	string ps_string = {};
	WCHARToString(ps_filename, ps_string);
	if (ps_index_container_.find(ps_string) != ps_index_container_.end()) {
		return true;
	}

	BlobPtr pixel_shader_blob = {};
	if (FAILED(D3DCompileFromFile(ps_filename, nullptr, nullptr, ps_entry_point_, ps_target_version_,
		compile_flags, 0, &pixel_shader_blob, &last_compile_error_
	))) {
		if (nullptr != last_compile_error_) {
			char err[256] = {};
			memcpy(err, last_compile_error_.Get()->GetBufferPointer(), last_compile_error_.Get()->GetBufferSize());
			assert(0);
		}
		return false;
	}

	pixel_shader_container.push_back(pixel_shader_blob);
	unsigned int index = pixel_shader_container.size() - 1;

	ps_index_container_.insert(make_pair(ps_string, index));

	return true;
}

void ShaderLoader::SetVSEntryPoint(LPCSTR entry_point) {
	vs_entry_point_ = entry_point;
}

void ShaderLoader::SetPSEntryPoint(LPCSTR entry_point) {
	ps_entry_point_ = entry_point;
}

void ShaderLoader::SetVSTargetVersion(LPCSTR target_version) {
	vs_target_version_ = target_version;
}

void ShaderLoader::SetPSTargetVersion(LPCSTR target_version) {
	ps_target_version_ = target_version;
}

BlobPtr ShaderLoader::GetVertexShaderBlobByIndex(unsigned int index) const {
	return vertex_shader_container.at(index);
}
			 
BlobPtr ShaderLoader::GetPixelShaderBlobByIndex(unsigned int index) const {
	return pixel_shader_container.at(index);
}
			  
BlobPtr ShaderLoader::GetVertexShaderBlobByFileName(WCHAR* filename) const {
	string s;
	WCHARToString(filename, s);
	unsigned int index = vs_index_container_.find(s)->second;
	return GetVertexShaderBlobByIndex(index);
}
			  
BlobPtr ShaderLoader::GetPixelShaderBlobByFileName(WCHAR* filename) const {
	string s;
	WCHARToString(filename, s);
	unsigned int index = ps_index_container_.find(s)->second;
	return GetPixelShaderBlobByIndex(index);
}
// TODO: finish these functions
BlobPtr ShaderLoader::GetVertexShaderBlobByEntryName(WCHAR* entry_name) const {
	return BlobPtr();
}
//			  
BlobPtr ShaderLoader::GetPixelShaderBlobByEntryName(WCHAR* entry_name) const {
	return BlobPtr();
}
