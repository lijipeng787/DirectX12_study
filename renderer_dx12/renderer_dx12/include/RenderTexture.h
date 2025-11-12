#pragma once

#include <memory>

#include "DirectX12Device.h"

class RenderTexture {
public:
  explicit RenderTexture(std::shared_ptr<DirectX12Device> device);

  RenderTexture(const RenderTexture &rhs) = delete;
  RenderTexture &operator=(const RenderTexture &rhs) = delete;

  ~RenderTexture();

  bool Initialize(UINT width, UINT height,
                  DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM);

  void Shutdown();

  bool BeginRender();

  void EndRender();

  DescriptorHeapPtr GetShaderResourceView() const;

  const RenderTargetDescriptor &GetDescriptor() const {
    return descriptor_;
  }

  ResourceSharedPtr GetTexture() const;

  RenderTargetHandle GetHandle() const { return render_target_handle_; }

private:
  std::shared_ptr<DirectX12Device> device_ = nullptr;

  RenderTargetHandle render_target_handle_ = kInvalidRenderTargetHandle;
  
  RenderTargetDescriptor descriptor_ = {};
};

