#pragma once

#include <cstring>
#include <memory>

#include "DirectX12Device.h"
#include "d3dx12.h"
#include "TypeDefine.h"

template <typename T>
class ConstantBuffer {
public:
  ConstantBuffer() = default;

  ConstantBuffer(const ConstantBuffer &) = delete;
  
  auto operator=(const ConstantBuffer &) -> ConstantBuffer & = delete;

  ConstantBuffer(ConstantBuffer &&) noexcept = default;
  
  auto operator=(ConstantBuffer &&) noexcept -> ConstantBuffer & = default;

  ~ConstantBuffer() = default;

  auto Initialize(const std::shared_ptr<DirectX12Device> &device) -> bool {
    if (!device) {
      return false;
    }

    auto d3d_device = device->GetD3d12Device();
    if (!d3d_device) {
      return false;
    }

    if (FAILED(d3d_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(CBSIZE(T)),
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&resource_)))) {
      return false;
    }

    std::memset(&data_, 0, sizeof(T));
    return true;
  }

  auto Update(const T &data) -> bool {
    if (!resource_) {
      return false;
    }

    UINT8 *mapped_data = nullptr;
    if (FAILED(resource_->Map(0, nullptr,
                              reinterpret_cast<void **>(&mapped_data)))) {
      return false;
    }

    data_ = data;
    std::memcpy(mapped_data, &data_, sizeof(T));
    resource_->Unmap(0, nullptr);
    return true;
  }

  auto GetResource() const -> ResourceSharedPtr { return resource_; }

  auto GetData() const -> const T & { return data_; }

private:
  ResourceSharedPtr resource_ = nullptr;
  
  T data_ = {};
};


