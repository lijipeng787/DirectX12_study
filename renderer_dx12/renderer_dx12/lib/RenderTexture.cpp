#include "stdafx.h"

#include "RenderTexture.h"

#include <utility>

RenderTexture::RenderTexture(std::shared_ptr<DirectX12Device> device)
    : device_(std::move(device)) {}

RenderTexture::~RenderTexture() { Shutdown(); }

bool RenderTexture::Initialize(UINT width, UINT height, DXGI_FORMAT format) {
  if (!device_) {
    return false;
  }

  Shutdown();

  RenderTargetDescriptor descriptor = {};
  descriptor.width = width;
  descriptor.height = height;
  descriptor.format = format;
  descriptor.create_rtv = true;
  descriptor.create_srv = true;
  descriptor.clear_color[0] = 0.0f;
  descriptor.clear_color[1] = 0.0f;
  descriptor.clear_color[2] = 0.0f;
  descriptor.clear_color[3] = 1.0f;

  descriptor_ = descriptor;

  render_target_handle_ = device_->CreateRenderTarget(descriptor);
  return render_target_handle_ != kInvalidRenderTargetHandle;
}

void RenderTexture::Shutdown() {
  if (device_ && render_target_handle_ != kInvalidRenderTargetHandle) {
    device_->DestroyRenderTarget(render_target_handle_);
    render_target_handle_ = kInvalidRenderTargetHandle;
  }
  descriptor_ = {};
}

bool RenderTexture::BeginRender() {
  if (!device_ || render_target_handle_ == kInvalidRenderTargetHandle) {
    return false;
  }

  device_->BeginDrawToOffScreen(render_target_handle_);
  return true;
}

void RenderTexture::EndRender() {
  if (!device_ || render_target_handle_ == kInvalidRenderTargetHandle) {
    return;
  }

  device_->EndDrawToOffScreen(render_target_handle_);
}

DescriptorHeapPtr RenderTexture::GetShaderResourceView() const {
  if (!device_ || render_target_handle_ == kInvalidRenderTargetHandle) {
    return nullptr;
  }

  return device_->GetRenderTargetSrv(render_target_handle_);
}

ResourceSharedPtr RenderTexture::GetTexture() const {
  if (!device_ || render_target_handle_ == kInvalidRenderTargetHandle) {
    return nullptr;
  }
  return device_->GetRenderTargetTexture(render_target_handle_);
}
