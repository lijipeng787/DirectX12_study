#include "stdafx.h"

#include "Texture.h"

bool Texture::set_texture(WCHAR **texture_filename_arr) {

	auto device = DirectX12Device::GetD3d12DeviceInstance()->GetD3d12Device();

	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = num_textures_;
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&shader_resource_view_heap_)))) {
		return false;
	}

	auto increasement_size = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE handle(shader_resource_view_heap_.Get()->GetCPUDescriptorHandleForHeapStart());
	ResourceSharedPtr tem_texture = nullptr;

	for (unsigned int index = 0; index < num_textures_; ++index) {

		if (FAILED(CreateDDSTextureFromFile(
			device.Get(),
			texture_filename_arr[index],
			0,
			false,
			&tem_texture,
			handle
		))) {
			return false;
		}
		handle.Offset(increasement_size);
		texture_.push_back(tem_texture);
	}

	return true;
}
