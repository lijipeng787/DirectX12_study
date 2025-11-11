#include "stdafx.h"

#include "DDSTextureLoader.h"
#include "DirectX12Device.h"
#include "TextureLoader.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace ResourceLoader {

namespace {

#pragma pack(push, 1)
struct TargaHeader {
  uint8_t id_length;
  uint8_t color_map_type;
  uint8_t data_type_code;
  uint16_t color_map_origin;
  uint16_t color_map_length;
  uint8_t color_map_depth;
  uint16_t x_origin;
  uint16_t y_origin;
  uint16_t width;
  uint16_t height;
  uint8_t bits_per_pixel;
  uint8_t image_descriptor;
};
#pragma pack(pop)

struct HandleCloser {
  explicit HandleCloser(HANDLE h) : handle(h) {}
  ~HandleCloser() {
    if (handle) {
      CloseHandle(handle);
    }
  }
  HANDLE Handle() const { return handle; }
  HANDLE handle = nullptr;
};

bool LoadTarga32Bit(const std::wstring &file_path, std::vector<uint8_t> &data,
                    uint32_t &width, uint32_t &height) {
  std::ifstream file(file_path, std::ios::binary);
  if (!file) {
    return false;
  }

  TargaHeader header = {};
  file.read(reinterpret_cast<char *>(&header), sizeof(TargaHeader));
  if (!file) {
    return false;
  }

  if (header.bits_per_pixel != 32) {
    return false;
  }

  if (header.id_length > 0) {
    file.seekg(header.id_length, std::ios::cur);
  }

  if (header.color_map_length > 0 && header.color_map_depth > 0) {
    size_t color_map_size =
        static_cast<size_t>(header.color_map_length) *
        static_cast<size_t>((header.color_map_depth + 7) / 8);
    file.seekg(static_cast<std::streamoff>(color_map_size), std::ios::cur);
  }

  width = header.width;
  height = header.height;

  size_t image_size =
      static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
  std::vector<uint8_t> raw_data(image_size);
  file.read(reinterpret_cast<char *>(raw_data.data()),
            static_cast<std::streamsize>(image_size));
  if (!file) {
    return false;
  }

  data.resize(image_size);

  const bool flip_vertical = ((header.image_descriptor & 0x20) == 0);
  for (uint32_t y = 0; y < height; ++y) {
    uint32_t src_row = flip_vertical ? (height - 1 - y) : y;
    const uint8_t *src =
        raw_data.data() + (static_cast<size_t>(src_row) * width * 4);
    uint8_t *dst = data.data() + (static_cast<size_t>(y) * width * 4);
    for (uint32_t x = 0; x < width; ++x) {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      dst[3] = src[3];
      dst += 4;
      src += 4;
    }
  }

  return true;
}

bool CreateTextureFromTga(ID3D12Device *device, const std::wstring &file_path,
                          ResourceSharedPtr &texture,
                          D3D12_CPU_DESCRIPTOR_HANDLE srv_handle) {
  std::vector<uint8_t> image_data = {};
  uint32_t width = 0;
  uint32_t height = 0;
  if (!LoadTarga32Bit(file_path, image_data, width, height)) {
    return false;
  }

  D3D12_RESOURCE_DESC resource_desc = {};
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resource_desc.Alignment = 0;
  resource_desc.Width = width;
  resource_desc.Height = height;
  resource_desc.DepthOrArraySize = 1;
  resource_desc.MipLevels = 1;
  resource_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  resource_desc.SampleDesc.Count = 1;
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  ResourceSharedPtr texture_resource = nullptr;
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
          D3D12_HEAP_FLAG_NONE, &resource_desc, D3D12_RESOURCE_STATE_COMMON,
          nullptr, IID_PPV_ARGS(&texture_resource)))) {
    return false;
  }

  UINT64 upload_size =
      GetRequiredIntermediateSize(texture_resource.Get(), 0, 1);

  ResourceSharedPtr upload_resource = nullptr;
  if (FAILED(device->CreateCommittedResource(
          &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
          D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(upload_size),
          D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
          IID_PPV_ARGS(&upload_resource)))) {
    return false;
  }

  D3D12_SUBRESOURCE_DATA subresource_data = {};
  subresource_data.pData = image_data.data();
  subresource_data.RowPitch = static_cast<LONG_PTR>(width) * 4;
  subresource_data.SlicePitch =
      static_cast<LONG_PTR>(subresource_data.RowPitch) * height;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue = nullptr;
  D3D12_COMMAND_QUEUE_DESC queue_desc = {};
  queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  if (FAILED(device->CreateCommandQueue(&queue_desc,
                                        IID_PPV_ARGS(&command_queue)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator = nullptr;
  if (FAILED(device->CreateCommandAllocator(
          D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)))) {
    return false;
  }

  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list = nullptr;
  if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       command_allocator.Get(), nullptr,
                                       IID_PPV_ARGS(&command_list)))) {
    return false;
  }

  auto to_copy = CD3DX12_RESOURCE_BARRIER::Transition(
      texture_resource.Get(), D3D12_RESOURCE_STATE_COMMON,
      D3D12_RESOURCE_STATE_COPY_DEST);
  command_list->ResourceBarrier(1, &to_copy);

  UpdateSubresources(command_list.Get(), texture_resource.Get(),
                     upload_resource.Get(), 0, 0, 1, &subresource_data);

  auto to_shader = CD3DX12_RESOURCE_BARRIER::Transition(
      texture_resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
      D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
  command_list->ResourceBarrier(1, &to_shader);

  if (FAILED(command_list->Close())) {
    return false;
  }

  ID3D12CommandList *command_lists[] = {command_list.Get()};
  command_queue->ExecuteCommandLists(1, command_lists);

  Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
  if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                                 IID_PPV_ARGS(&fence)))) {
    return false;
  }

  HandleCloser fence_event(CreateEvent(nullptr, FALSE, FALSE, nullptr));
  if (!fence_event.Handle()) {
    return false;
  }

  UINT64 fence_value = 1;
  if (FAILED(command_queue->Signal(fence.Get(), fence_value))) {
    return false;
  }

  if (fence->GetCompletedValue() < fence_value) {
    if (FAILED(
            fence->SetEventOnCompletion(fence_value, fence_event.Handle()))) {
      return false;
    }
    WaitForSingleObject(fence_event.Handle(), INFINITE);
  }

  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  device->CreateShaderResourceView(texture_resource.Get(), &srv_desc,
                                   srv_handle);

  texture = texture_resource;
  return true;
}

std::wstring ToLower(std::wstring value) {
  std::transform(value.begin(), value.end(), value.begin(), [](wchar_t c) {
    return static_cast<wchar_t>(std::towlower(c));
  });
  return value;
}

bool EndsWith(const std::wstring &value, const std::wstring &suffix) {
  if (value.length() < suffix.length()) {
    return false;
  }
  return value.compare(value.length() - suffix.length(), suffix.length(),
                       suffix) == 0;
}

} // namespace

TextureLoader::TextureLoader(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

bool TextureLoader::LoadTextureByName(WCHAR **texture_filename) {
  return false;
}

bool TextureLoader::LoadTexturesByNameArray(unsigned int num_textures,
                                            WCHAR **texture_filename_arr) {

  auto device = device_->GetD3d12Device();

  D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
  srv_heap_desc.NumDescriptors = num_textures;
  srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  if (FAILED(device->CreateDescriptorHeap(
          &srv_heap_desc, IID_PPV_ARGS(&shader_resource_view_heap_)))) {
    return false;
  }

  auto increasement_size = device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
      shader_resource_view_heap_.Get()->GetCPUDescriptorHandleForHeapStart());
  ResourceSharedPtr tem_texture = nullptr;
  auto index = 0;
  string filename = {};

  for (unsigned int i = 0; i < num_textures; ++i) {
    tem_texture.Reset();
    std::wstring file_path(texture_filename_arr[i]);
    std::wstring lowercase = ToLower(file_path);

    bool load_result = false;
    if (EndsWith(lowercase, L".dds")) {
      load_result = SUCCEEDED(
          CreateDDSTextureFromFile(device.Get(), texture_filename_arr[i], 0,
                                   false, &tem_texture, handle));
    } else if (EndsWith(lowercase, L".tga")) {
      load_result =
          CreateTextureFromTga(device.Get(), file_path, tem_texture, handle);
    } else {
      return false;
    }

    if (!load_result) {
      return false;
    }

    texture_container_.push_back(tem_texture);

    filename.clear();
    WCHARToString(texture_filename_arr[i], filename);
    index_container_.insert(make_pair(
        filename, static_cast<unsigned int>(texture_container_.size() - 1)));

    handle.Offset(increasement_size);
  }

  return true;
}

ResourceSharedPtr TextureLoader::GetTextureResource(size_t index) const {
  if (index >= texture_container_.size()) {
    return nullptr;
  }
  return texture_container_[index];
}
} // namespace ResourceLoader
