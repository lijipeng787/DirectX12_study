#ifndef _LIGHTSHADERCLASS_H_
#define _LIGHTSHADERCLASS_H_

#include <string>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include <d3dcompiler.h>

typedef Microsoft::WRL::ComPtr<ID3DBlob> BlobPtr;

typedef std::vector<BlobPtr> VSBlobVector;

typedef std::vector<BlobPtr> PSBlobVector;

typedef std::unordered_map<std::string, int> VSIndexContainer;

typedef std::unordered_map<std::string, int> PSIndexContainer;

namespace ResourceLoader {

		class ShaderLoader {
		public:
			ShaderLoader() {}

			ShaderLoader(const ShaderLoader& rhs) = delete;

			ShaderLoader& operator==(const ShaderLoader& rhs) = delete;

			~ShaderLoader() {}
		public:
			bool CreateVSAndPSFromFile(WCHAR *vs_shader_filename, WCHAR *ps_shader_filename);

			bool CreateVSFromFile(WCHAR *vs_shader_filename);

			bool CreatePSFromFile(WCHAR *ps_shader_filename);

			void SetVSEntryPoint(LPCSTR entry_point);

			void SetPSEntryPoint(LPCSTR entry_point);

			void SetVSTargetVersion(LPCSTR target_version);

			void SetPSTargetVersion(LPCSTR target_version);

			const BlobPtr& GetVertexShaderBlobByIndex(int index) const;

			const BlobPtr& GetPixelShaderBlobByIndex(int index) const;

			const BlobPtr& GetVertexShaderBlobByFileName(WCHAR* filename) const;

			const BlobPtr& GetPixelShaderBlobByFileName(WCHAR* filename) const;

			const BlobPtr& GetVertexShaderBlobByEntryName(WCHAR* entry_name) const;

			const BlobPtr& GetPixelShaderBlobByEntryName(WCHAR* entry_name) const;
		private:
			VSIndexContainer vs_index_container_ = {};

			PSIndexContainer ps_index_container_ = {};
			
			VSBlobVector vertex_shader_container = {};

			PSBlobVector pixel_shader_container = {};

			BlobPtr last_compile_error_ = nullptr;

			LPCSTR vs_entry_point = {};

			LPCSTR ps_entry_point = {};

			LPCSTR vs_target_version = {};

			LPCSTR ps_target_version = {};
	};

}
#endif