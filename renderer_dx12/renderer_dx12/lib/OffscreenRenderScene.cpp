#include "stdafx.h"

#include "OffscreenRenderScene.h"

#include "Camera.h"
#include "DirectX12Device.h"
#include "LightManager.h"
#include "ShaderLoader.h"

using namespace DirectX;

auto OffscreenRenderScene::Initialize(const SceneInitializeContext &ctx)
    -> bool {
  device_ = ctx.device;
  shader_loader_ = ctx.shader_loader;

  if (!device_ || !shader_loader_ || !ctx.camera || !ctx.light_manager) {
    return false;
  }

  // Store base view matrix for text rendering
  base_view_matrix_ = ctx.camera->GetViewMatrix();

  // Create model
  model_ = std::make_shared<Model>(device_);
  if (!model_) {
    return false;
  }

  auto model_material = model_->GetMaterial();
  model_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/light.hlsl")
          .Get()));
  model_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/light.hlsl")
          .Get()));

  WCHAR *model_textures[3] = {L"data/stone01.dds", L"data/dirt01.dds",
                              L"data/alpha01.dds"};
  if (!model_->Initialize(L"data/cube.txt", model_textures)) {
    return false;
  }

  // Note: Text object will be shared with TextOverlayScene
  // It will be set via SetSharedText() after both scenes are initialized

  return true;
}

void OffscreenRenderScene::Shutdown() {
  // Don't reset text_ here - it's shared with TextOverlayScene
  text_ = nullptr;  // Just clear the reference
  model_.reset();
}

void OffscreenRenderScene::Update(float /*delta_seconds*/) {
  // Content is updated externally (FPS/CPU) or via SetRotationAngle
}

auto OffscreenRenderScene::RenderReflection(const SceneReflectionContext &ctx)
    -> bool {
  if (!device_ || !model_) {
    return false;
  }

  auto model_material = model_->GetMaterial();
  if (!model_material) {
    return false;
  }

  // Text is optional (shared with TextOverlayScene)
  TextMaterial* text_material = nullptr;
  if (text_) {
    text_material = text_->GetMaterial();
  }

  // Cache resources on first render
  if (!cached_resources_.light_root_signature) {
    cached_resources_.light_root_signature =
        model_material->GetRootSignature().Get();
    cached_resources_.light_pso =
        model_material->GetPSOByName("model_normal").Get();
    cached_resources_.light_matrix_cb =
        model_material->GetMatrixConstantBuffer().Get();
    cached_resources_.light_cb =
        model_material->GetLightConstantBuffer().Get();
    cached_resources_.fog_cb =
        model_material->GetFogConstantBuffer().Get();
  }

  // Cache text resources if text is available
  if (text_material && !cached_resources_.font_root_signature) {
    cached_resources_.font_root_signature =
        text_material->GetRootSignature().Get();
    cached_resources_.font_pso =
        text_material->GetPSOByName("text_blend_enable").Get();
    cached_resources_.font_matrix_cb =
        text_material->GetMatrixConstantBuffer().Get();
    cached_resources_.font_pixel_cb =
        text_material->GetPixelConstantBuffer().Get();
  }

  // Calculate matrices for model
  XMMATRIX world_matrix = {};
  device_->GetWorldMatrix(world_matrix);

  XMMATRIX rotate_world =
      XMMatrixTranspose(XMMatrixRotationY(rotation_angle_) *
                        XMMatrixTranslation(-6.0f, 1.5f, -6.0f));

  XMMATRIX view_matrix = XMMatrixTranspose(XMMatrixIdentity());
  XMMATRIX projection = XMMatrixTranspose(ctx.projection);

  // Update model constant buffers
  if (!model_material->UpdateMatrixConstant(rotate_world, view_matrix,
                                            projection)) {
    return false;
  }

  // Get light (already set by Graphics::Initialize)
  // Light constant buffer should already be updated

  if (!model_material->UpdateFogConstant(3.0f, 6.0f)) {
    return false;
  }

  // Update text constant buffers if text is available
  if (text_material) {
    XMMATRIX font_world = XMMatrixTranspose(world_matrix);
    XMMATRIX base_view = XMMatrixTranspose(base_view_matrix_);
    XMMATRIX orthogonality = {};
    device_->GetOrthoMatrix(orthogonality);
    orthogonality = XMMatrixTranspose(orthogonality);

    if (!text_material->UpdateMatrixConstant(font_world, base_view,
                                             orthogonality)) {
      return false;
    }

    XMFLOAT4 pixel_color(1.0f, 0.0f, 0.0f, 0.0f);
    if (!text_material->UpdateLightConstant(pixel_color)) {
      return false;
    }
  }

  // Begin offscreen rendering
  device_->BeginDrawToOffScreen();

  // Render model
  device_->SetGraphicsRootSignature(cached_resources_.light_root_signature);
  device_->SetPipelineStateObject(cached_resources_.light_pso);

  ID3D12DescriptorHeap *light_shader_heap[] = {
      model_->GetShaderResourceView().Get()};
  device_->SetDescriptorHeaps(1, light_shader_heap);

  device_->SetGraphicsRootDescriptorTable(
      0, light_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, cached_resources_.light_matrix_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(
      2, cached_resources_.light_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(
      3, cached_resources_.fog_cb->GetGPUVirtualAddress());

  device_->BindIndexBuffer(&model_->GetIndexBufferView());
  device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());
  device_->Draw(model_->GetIndexCount());

  // Render text if available (shared with TextOverlayScene)
  if (text_ && cached_resources_.font_root_signature) {
    device_->SetGraphicsRootSignature(cached_resources_.font_root_signature);
    device_->SetPipelineStateObject(cached_resources_.font_pso);

    ID3D12DescriptorHeap *font_shader_heap[] = {
        text_->GetShaderResourceView().Get()};
    device_->SetDescriptorHeaps(1, font_shader_heap);

    device_->SetGraphicsRootDescriptorTable(
        0, font_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    device_->SetGraphicsRootConstantBufferView(
        1, cached_resources_.font_matrix_cb->GetGPUVirtualAddress());
    device_->SetGraphicsRootConstantBufferView(
        2, cached_resources_.font_pixel_cb->GetGPUVirtualAddress());

    // Render both text sections
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
  }

  // End offscreen rendering
  device_->EndDrawToOffScreen();

  return true;
}

