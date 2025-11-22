#include "stdafx.h"

#include "OffscreenPreviewScene.h"

#include "DirectX12Device.h"
#include "ScreenQuadMaterial.h"
#include "ShaderLoader.h"
#include <Camera.h>

using namespace DirectX;

auto OffscreenPreviewScene::Initialize(const SceneInitializeContext &ctx)
    -> bool {
  device_ = ctx.device;
  shader_loader_ = ctx.shader_loader;

  if (!device_ || !shader_loader_ || !ctx.camera) {
    return false;
  }

  int screen_width = device_->GetScreenWidth();
  int screen_height = device_->GetScreenHeight();

  // Create bitmap material
  auto bitmap_material = std::make_shared<ScreenQuadMaterial>(device_);
  if (!bitmap_material) {
    return false;
  }

  bitmap_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/texture.hlsl")
          .Get()));
  bitmap_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/texture.hlsl")
          .Get()));

  // Create screen quad
  bitmap_ = std::make_shared<ScreenQuad>(device_, bitmap_material);
  if (!bitmap_) {
    return false;
  }

  if (!bitmap_->Initialize(screen_width, screen_height, 255, 255)) {
    return false;
  }

  // Store base view matrix for orthographic rendering
  base_view_matrix_ = ctx.camera.get()->GetViewMatrix();

  return true;
}

void OffscreenPreviewScene::Shutdown() { bitmap_.reset(); }

void OffscreenPreviewScene::Update(float /*delta_seconds*/) {
  // Preview doesn't need per-frame updates
}

auto OffscreenPreviewScene::Render(const SceneRenderContext &ctx) -> bool {
  if (!device_ || !bitmap_) {
    return false;
  }

  auto material = bitmap_->GetMaterial();
  if (!material) {
    return false;
  }

  // Build matrices for orthographic rendering
  XMMATRIX world_matrix = {};
  device_->GetWorldMatrix(world_matrix);
  XMMATRIX font_world = XMMatrixTranspose(world_matrix);
  XMMATRIX base_view = XMMatrixTranspose(base_view_matrix_);

  XMMATRIX orthogonality = {};
  device_->GetOrthoMatrix(orthogonality);
  orthogonality = XMMatrixTranspose(orthogonality);

  // Update constant buffer
  material->UpdateConstantBuffer(font_world, base_view, orthogonality);

  // Update position
  bitmap_->UpdatePosition(position_x_, position_y_);

  // Get rendering resources
  auto root_signature = material->GetRootSignature();
  auto pso = material->GetPSOByName("bitmap_normal");
  if (!root_signature || !pso) {
    return false;
  }

  auto constant_buffer = material->GetConstantBuffer();
  if (!constant_buffer) {
    return false;
  }

  // Set pipeline state
  device_->SetGraphicsRootSignature(root_signature);
  device_->SetPipelineStateObject(pso);

  // Bind offscreen texture
  auto off_screen_heap = device_->GetOffScreenTextureHeapView();
  ID3D12DescriptorHeap *off_screen_descriptor_heap[] = {off_screen_heap.Get()};
  device_->SetDescriptorHeaps(1, off_screen_descriptor_heap);

  device_->SetGraphicsRootDescriptorTable(
      0, off_screen_descriptor_heap[0]->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, constant_buffer->GetGPUVirtualAddress());

  // Draw
  device_->BindVertexBuffer(0, 1, &bitmap_->GetVertexBufferView());
  device_->BindIndexBuffer(&bitmap_->GetIndexBufferView());
  device_->Draw(bitmap_->GetIndexCount());

  return true;
}

