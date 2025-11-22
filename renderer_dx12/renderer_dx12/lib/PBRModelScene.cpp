#include "stdafx.h"

#include "PBRModelScene.h"

#include "Camera.h"
#include "DirectX12Device.h"
#include "LightManager.h"
#include "SceneLight.h"
#include "ShaderLoader.h"

using namespace DirectX;

auto PBRModelScene::Initialize(const SceneInitializeContext &ctx) -> bool {
  device_ = ctx.device;
  shader_loader_ = ctx.shader_loader;
  camera_ = ctx.camera;

  if (!device_ || !shader_loader_ || !camera_) {
    return false;
  }

  // Create PBR model
  model_ = std::make_shared<PBRModel>(device_);
  if (!model_) {
    return false;
  }

  // Set shader bytecodes
  PBRMaterial *pbr_material = model_->GetMaterial();
  pbr_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/pbr.hlsl")
          .Get()));
  pbr_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/pbr.hlsl")
          .Get()));

  // Load PBR model and textures
  WCHAR *pbr_textures[3] = {L"data/pbr/pbr_albedo.tga",
                            L"data/pbr/pbr_normal.tga",
                            L"data/pbr/pbr_roughmetal.tga"};
  if (!model_->Initialize(L"data/pbr/sphere.txt", pbr_textures)) {
    return false;
  }

  return true;
}

void PBRModelScene::Shutdown() { model_.reset(); }

void PBRModelScene::Update(float /*delta_seconds*/) {
  // PBR model doesn't need per-frame updates (rotation handled via
  // SetRotationAngle)
}

auto PBRModelScene::Render(const SceneRenderContext &ctx) -> bool {
  if (!device_ || !model_ || !camera_) {
    return false;
  }

  auto material = model_->GetMaterial();
  if (!material) {
    return false;
  }

  // Build world matrix
  XMMATRIX world = XMMatrixRotationY(rotation_radians_) *
                   XMMatrixTranslation(kModelPosition.x, kModelPosition.y,
                                       kModelPosition.z);
  XMMATRIX world_t = XMMatrixTranspose(world);
  XMMATRIX view_t = XMMatrixTranspose(ctx.view);
  XMMATRIX projection_t = XMMatrixTranspose(ctx.projection);

  // Update constant buffers
  if (!material->UpdateMatrixConstant(world_t, view_t, projection_t)) {
    return false;
  }

  XMFLOAT3 camera_position = camera_->GetPosition();
  if (!material->UpdateCameraConstant(camera_position)) {
    return false;
  }

  if (ctx.primary_light) {
    if (!material->UpdateFromLight(ctx.primary_light)) {
      return false;
    }
  }

  // Get rendering resources
  auto root_signature = material->GetRootSignature();
  auto pso = material->GetPSOByName("pbr_pipeline");
  if (!root_signature || !pso) {
    return false;
  }

  auto descriptor_heap = model_->GetShaderResourceView();
  auto matrix_cb = material->GetMatrixConstantBuffer();
  auto camera_cb = material->GetCameraConstantBuffer();
  auto light_cb = material->GetLightConstantBuffer();
  if (!descriptor_heap || !matrix_cb || !camera_cb || !light_cb) {
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
      2, camera_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(3,
                                             light_cb->GetGPUVirtualAddress());

  // Draw
  device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());
  device_->BindIndexBuffer(&model_->GetIndexBufferView());
  device_->Draw(model_->GetIndexCount());

  return true;
}

void PBRModelScene::SetRotationAngle(float radians) {
  rotation_radians_ = radians;
}

