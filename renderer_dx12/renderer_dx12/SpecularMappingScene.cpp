#include "stdafx.h"

#include "SpecularMappingScene.h"

#include "Camera.h"
#include "DirectX12Device.h"
#include "LightManager.h"
#include "SceneLight.h"
#include "ShaderLoader.h"

using namespace DirectX;
using namespace Lighting;
using namespace ResourceLoader;

SpecularMappingScene::SpecularMappingScene(
    std::shared_ptr<DirectX12Device> device,
    std::shared_ptr<ShaderLoader> shader_loader,
    std::shared_ptr<LightManager> light_manager,
    std::shared_ptr<Camera> camera)
    : device_(std::move(device)),
      shader_loader_(std::move(shader_loader)),
      light_manager_(std::move(light_manager)),
      camera_(std::move(camera)) {}

auto SpecularMappingScene::Initialize() -> bool {
  if (!device_ || !shader_loader_ || !light_manager_ || !camera_) {
    return false;
  }

  if (!EnsureShadersLoaded()) {
    return false;
  }

  model_ = std::make_shared<SpecularMapModel>(device_);
  if (!model_) {
    return false;
  }

  auto material = model_->GetMaterial();
  material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/specMap.hlsl")
          .Get()));
  material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/specMap.hlsl")
          .Get()));

  WCHAR *textures[3] = {L"data/stone02.dds", L"data/bump02.dds",
                        L"data/spec02.dds"};
  if (!model_->Initialize(L"data/cube.txt", textures, 3)) {
    return false;
  }

  return true;
}

void SpecularMappingScene::Shutdown() {
  model_.reset();
  shaders_loaded_ = false;
}

void SpecularMappingScene::Update(float /*delta_seconds*/) {}

auto SpecularMappingScene::Render(const XMMATRIX &view,
                                  const XMMATRIX &projection,
                                  const SceneLight *scene_light) -> bool {
  if (!device_ || !model_) {
    return false;
  }

  auto material = model_->GetMaterial();
  if (!material) {
    return false;
  }

  XMMATRIX world = XMMatrixRotationY(rotation_radians_) *
                   XMMatrixTranslation(position_.x, position_.y, position_.z);

  XMMATRIX world_t = XMMatrixTranspose(world);
  XMMATRIX view_t = XMMatrixTranspose(view);
  XMMATRIX projection_t = XMMatrixTranspose(projection);

  if (!material->UpdateMatrixConstant(world_t, view_t, projection_t)) {
    return false;
  }

  XMFLOAT3 camera_position = camera_->GetPosition();
  if (!material->UpdateCameraConstant(camera_position)) {
    return false;
  }

  const SceneLight *light_to_use = scene_light;
  if (!light_to_use && light_manager_) {
    light_to_use = light_manager_->GetPrimaryLight().get();
  }

  if (light_to_use) {
    if (!material->UpdateLightFromScene(light_to_use)) {
      return false;
    }
  }

  auto root_signature = material->GetRootSignature();
  auto pso = material->GetPSOByName("specular_map_main");
  if (!root_signature || !pso) {
    return false;
  }

  auto descriptor_heap = model_->GetShaderResourceView();
  if (!descriptor_heap) {
    return false;
  }

  auto matrix_cb = material->GetMatrixConstantBuffer();
  auto camera_cb = material->GetCameraConstantBuffer();
  auto light_cb = material->GetLightConstantBuffer();
  if (!matrix_cb || !camera_cb || !light_cb) {
    return false;
  }

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
  device_->SetGraphicsRootConstantBufferView(
      3, light_cb->GetGPUVirtualAddress());

  device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());
  device_->BindIndexBuffer(&model_->GetIndexBufferView());
  device_->Draw(model_->GetIndexCount());

  return true;
}

void SpecularMappingScene::SetRotationAngle(float radians) {
  rotation_radians_ = radians;
}

auto SpecularMappingScene::EnsureShadersLoaded() -> bool {
  if (shaders_loaded_) {
    return true;
  }

  if (!shader_loader_) {
    return false;
  }

  shader_loader_->SetVSEntryPoint("SpecMapVertexShader");
  shader_loader_->SetPSEntryPoint("SpecMapPixelShader");

  if (!shader_loader_->CreateVSAndPSFromFile(L"shader/specMap.hlsl",
                                             L"shader/specMap.hlsl")) {
    return false;
  }

  shaders_loaded_ = true;
  return true;
}

