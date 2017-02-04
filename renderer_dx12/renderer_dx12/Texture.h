#ifndef HEADER_TEXTURECLASS_H
#define HEADER_TEXTURECLASS_H

#include "DirectX12Device.h"

class Texture {
public:
	Texture() {}

	Texture(const Texture& rhs) = delete;

	Texture& operator=(const Texture& rhs) = delete;

	~Texture() {}
public:
	const DescriptorHeapPtr& get_textures_descriptor_heap()const { return shader_resource_view_heap_; }

	void set_texture_array_capacity(unsigned int num_textures) { num_textures_ = num_textures; }

	bool set_texture(WCHAR **texture_filename_arr);
private:
	unsigned int num_textures_ = 1;

	std::vector<ResourceSharedPtr> texture_ = {};

	DescriptorHeapPtr shader_resource_view_heap_ = nullptr;
};

#endif
