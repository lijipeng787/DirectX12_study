#include "stdafx.h"

#include "LegacyModelScene.h"

#include "DirectX12Device.h"
#include "LightManager.h"
#include "SceneLight.h"
#include "ShaderLoader.h"

using namespace DirectX;

auto LegacyModelScene::Initialize(const SceneInitializeContext &ctx) -> bool {
  device_ = ctx.device;
  shader_loader_ = ctx.shader_loader;

  if (!device_ || !shader_loader_) {
    return false;
  }

  // Create model
  model_ = std::make_shared<Model>(device_);
  if (!model_) {
    return false;
  }

  // Set shader bytecodes
  ModelMaterial *model_material = model_->GetMaterial();
  model_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/light.hlsl")
          .Get()));
  model_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/light.hlsl")
          .Get()));

  // Load model and textures
  WCHAR *texture_filename_arr[3] = {L"data/stone01.dds", L"data/dirt01.dds",
                                    L"data/alpha01.dds"};
  if (!model_->Initialize(L"data/cube.txt", texture_filename_arr)) {
    return false;
  }

  return true;
}

void LegacyModelScene::Shutdown() { model_.reset(); }

void LegacyModelScene::Update(float /*delta_seconds*/) {
  // Model doesn't need per-frame updates (rotation handled via
  // SetRotationAngle)
}

auto LegacyModelScene::Render(const SceneRenderContext &ctx) -> bool {
  if (!device_ || !model_) {
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

  if (ctx.primary_light) {
    if (!material->UpdateFromLight(ctx.primary_light)) {
      return false;
    }
  }

  if (!material->UpdateFogConstant(3.0f, 6.0f)) {
    return false;
  }

  // Get rendering resources
  auto root_signature = material->GetRootSignature();
  auto pso = material->GetPSOByName("model_normal");
  if (!root_signature || !pso) {
    return false;
  }

  auto descriptor_heap = model_->GetShaderResourceView();
  auto matrix_cb = material->GetMatrixConstantBuffer();
  auto light_cb = material->GetLightConstantBuffer();
  auto fog_cb = material->GetFogConstantBuffer();
  if (!descriptor_heap || !matrix_cb || !light_cb || !fog_cb) {
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
      2, light_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(3,
                                             fog_cb->GetGPUVirtualAddress());

  // Draw
  device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());
  device_->BindIndexBuffer(&model_->GetIndexBufferView());
  device_->Draw(model_->GetIndexCount());

  return true;
}

void LegacyModelScene::SetRotationAngle(float radians) {
  rotation_radians_ = radians;
}

