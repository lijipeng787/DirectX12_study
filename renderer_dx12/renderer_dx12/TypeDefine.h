#pragma once

#ifndef _TYPEDEFINE_H_
#define _TYPEDEFINE_H_

#include <string>
#include <vector>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <wrl.h>

namespace Effect {}

namespace ResourceLoader {
	
	void WCHARToString(WCHAR* wchar, std::string& s) {

		wchar_t * wText = wchar;
		DWORD length_of_wchar = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);
		char *tem = new char[length_of_wchar];
		WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, tem, length_of_wchar, NULL, FALSE);
		s = tem;
		delete[]tem;
	}

	// TODO: finish this function
	//void StringToWCHAR(std::string s, WCHAR* wchar) {

	//string temp = s;
	//int length_of_string = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL, 0);
	//wchar_t * wszUtf8 = new wchar_t[length_of_string + 1];
	//memset(wszUtf8, 0, length_of_string * 2 + 2);
	//MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, length_of_string);
	//memcpy(wchar, wszUtf8, length_of_string + 1);
	//delete[] wszUtf8;
	//}

}

#define CHECK(return_value) ((return_value==false)?true:false)

#define CBSIZE(constant_buffer) ((sizeof(constant_buffer) + 255) & ~255)

typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescriptorHeapPtr;

typedef std::vector<D3D12_VIEWPORT> ViewPortVector;

typedef std::vector<D3D12_RECT> ScissorRectVector;

typedef Microsoft::WRL::ComPtr<IDXGISwapChain3> SwapChainPtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugDevice> D2d12DebugDevicePtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugCommandQueue> D3d12DebugCommandQueuePtr;

typedef Microsoft::WRL::ComPtr<ID3D12DebugCommandList> D3d12DebugCommandListPtr;

typedef Microsoft::WRL::ComPtr<ID3D12Device> D3d12DevicePtr;

typedef Microsoft::WRL::ComPtr<ID3D12Resource> ResourceSharedPtr;

typedef Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocatorPtr;

typedef Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueuePtr;

typedef Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignaturePtr;

typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineStateObjectPtr;

typedef Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GraphicsCommandListPtr;

typedef Microsoft::WRL::ComPtr<ID3D12Fence> FencePtr;

typedef D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

typedef D3D12_SHADER_BYTECODE VertexShaderByteCode;

typedef D3D12_SHADER_BYTECODE PixelShaderByteCode;

typedef D3D12_INDEX_BUFFER_VIEW IndexBufferView;

#endif // !_TYPEDEFINE_H_
