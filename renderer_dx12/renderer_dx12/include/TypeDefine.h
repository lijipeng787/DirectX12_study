#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <string>
#include <vector>
#include <wrl.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

namespace Effect {}

namespace ResourceLoader {

void WCHARToString(WCHAR *wchar, std::string &s);
// TODO: finish this function
void StringToWCHAR(std::string s, WCHAR *wchar);

} // namespace ResourceLoader

// CHECK macro removed - use direct logical negation (!) instead
// Old implementation was confusing: CHECK(x) meant "x failed"
// Now use: if (!function()) { return false; }

#define CBSIZE(constant_buffer) ((sizeof(constant_buffer) + 255) & ~255)

using DescriptorHeapPtr = Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>;

using ViewPortVector = std::vector<D3D12_VIEWPORT>;

using ScissorRectVector = std::vector<D3D12_RECT>;

using SwapChainPtr = Microsoft::WRL::ComPtr<IDXGISwapChain3>;

using D3d12DebugDevicePtr = Microsoft::WRL::ComPtr<ID3D12DebugDevice>;

using D3d12DebugCommandQueuePtr = Microsoft::WRL::ComPtr<ID3D12DebugCommandQueue>;

using D3d12DebugCommandListPtr = Microsoft::WRL::ComPtr<ID3D12DebugCommandList>;

using D3d12DevicePtr = Microsoft::WRL::ComPtr<ID3D12Device>;

using ResourceSharedPtr = Microsoft::WRL::ComPtr<ID3D12Resource>;

using CommandAllocatorPtr = Microsoft::WRL::ComPtr<ID3D12CommandAllocator>;

using CommandQueuePtr = Microsoft::WRL::ComPtr<ID3D12CommandQueue>;

using RootSignaturePtr = Microsoft::WRL::ComPtr<ID3D12RootSignature>;

using PipelineStateObjectPtr = Microsoft::WRL::ComPtr<ID3D12PipelineState>;

using GraphicsCommandListPtr = Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>;

using FencePtr = Microsoft::WRL::ComPtr<ID3D12Fence>;

using VertexBufferView = D3D12_VERTEX_BUFFER_VIEW;

using VertexShaderByteCode = D3D12_SHADER_BYTECODE;

using PixelShaderByteCode = D3D12_SHADER_BYTECODE;

using IndexBufferView = D3D12_INDEX_BUFFER_VIEW;

