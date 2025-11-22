#include "stdafx.h"

#include "TextOverlayScene.h"

#include "DirectX12Device.h"
#include "ShaderLoader.h"
#include <Camera.h>

using namespace DirectX;

auto TextOverlayScene::Initialize(const SceneInitializeContext &ctx) -> bool {
  device_ = ctx.device;
  shader_loader_ = ctx.shader_loader;

  if (!device_ || !shader_loader_ || !ctx.camera) {
    return false;
  }

  screen_width_ = device_->GetScreenWidth();
  screen_height_ = device_->GetScreenHeight();

  // Create text renderer
  text_ = std::make_shared<Text>(device_);
  if (!text_) {
    return false;
  }

  // Set shader bytecodes
  TextMaterial *text_material = text_->GetMaterial();
  text_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/font.hlsl")
          .Get()));
  text_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/font.hlsl")
          .Get()));

  // Load font
  WCHAR *font_texture[1] = {L"data/font.dds"};
  if (!text_->LoadFont(L"data/fontdata.txt", font_texture)) {
    return false;
  }

  // Initialize text with base view matrix from camera
  base_view_matrix_ = ctx.camera.get()->GetViewMatrix();
  if (!text_->Initialize(screen_width_, screen_height_, base_view_matrix_)) {
    return false;
  }

  return true;
}

void TextOverlayScene::Shutdown() { text_.reset(); }

void TextOverlayScene::Update(float /*delta_seconds*/) {
  // Text content is updated externally via GetText()->SetFps() etc.
}

auto TextOverlayScene::Render(const SceneRenderContext &ctx) -> bool {
  if (!device_ || !text_) {
    return false;
  }

  auto material = text_->GetMaterial();
  if (!material) {
    return false;
  }

  // Build matrices for orthographic text rendering
  XMMATRIX world_matrix = {};
  device_->GetWorldMatrix(world_matrix);
  XMMATRIX font_world = XMMatrixTranspose(world_matrix);
  XMMATRIX base_view = XMMatrixTranspose(base_view_matrix_);

  XMMATRIX orthogonality = {};
  device_->GetOrthoMatrix(orthogonality);
  orthogonality = XMMatrixTranspose(orthogonality);

  // Update constant buffers
  if (!material->UpdateMatrixConstant(font_world, base_view, orthogonality)) {
    return false;
  }

  // Set text color (red)
  XMFLOAT4 pixel_color(1.0f, 0.0f, 0.0f, 0.0f);
  if (!material->UpdateLightConstant(pixel_color)) {
    return false;
  }

  // Get rendering resources
  auto root_signature = material->GetRootSignature();
  auto pso = material->GetPSOByName("text_blend_enable");
  if (!root_signature || !pso) {
    return false;
  }

  auto descriptor_heap = text_->GetShaderResourceView();
  auto matrix_cb = material->GetMatrixConstantBuffer();
  auto pixel_cb = material->GetPixelConstantBuffer();
  if (!descriptor_heap || !matrix_cb || !pixel_cb) {
    return false;
  }

  // Set pipeline state
  device_->SetGraphicsRootSignature(root_signature);
  device_->SetPipelineStateObject(pso);

  ID3D12DescriptorHeap *heaps[] = {descriptor_heap.Get()};
  device_->SetDescriptorHeaps(1, heaps);

  device_->SetGraphicsRootDescriptorTable(
      0, descriptor_heap->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, matrix_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(
      2, pixel_cb->GetGPUVirtualAddress());

  // Render both text sections (FPS and CPU)
  auto vertex1 = text_->GetVertexBufferView(0);
  auto vertex2 = text_->GetVertexBufferView(1);
  auto index1 = text_->GetIndexBufferView(0);
  auto index2 = text_->GetIndexBufferView(1);

  device_->BindIndexBuffer(&index1);
  device_->BindVertexBuffer(0, 1, &vertex1);
  device_->Draw(text_->GetIndexCount(0));

  device_->BindIndexBuffer(&index2);
  device_->BindVertexBuffer(0, 1, &vertex2);
  device_->Draw(text_->GetIndexCount(1));

  return true;
}

