#include "stdafx.h"

#include "ReflectionScene.h"

#include <algorithm>
#include <utility>

#include "Camera.h"
#include "DirectX12Device.h"
#include "ShaderLoader.h"

using namespace DirectX;
using namespace ResourceLoader;

ReflectionScene::ReflectionScene(
    std::shared_ptr<DirectX12Device> device,
    std::shared_ptr<ShaderLoader> shader_loader,
    std::shared_ptr<Camera> camera)
    : device_(std::move(device)),
      shader_loader_(std::move(shader_loader)),
      camera_(std::move(camera)) {}

auto ReflectionScene::EnsureShadersLoaded() -> bool {
  if (shaders_loaded_) {
    return true;
  }

  if (!shader_loader_) {
    return false;
  }

  ResourceLoader::ShaderCompileDesc vs_desc{L"shader/reflection.hlsl",
                                            "ReflectionVertexShader",
                                            "vs_5_0"};
  ResourceLoader::ShaderCompileDesc ps_desc{L"shader/reflection.hlsl",
                                            "ReflectionPixelShader",
                                            "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(vs_desc, ps_desc)) {
    return false;
  }

  shaders_loaded_ = true;
  return true;
}

auto ReflectionScene::Initialize() -> bool {
  if (!device_ || !shader_loader_ || !camera_) {
    return false;
  }

  if (!EnsureShadersLoaded()) {
    return false;
  }

  render_texture_ = std::make_unique<RenderTexture>(device_);
  if (!render_texture_) {
    return false;
  }

  if (!render_texture_->Initialize(
          static_cast<UINT>(std::max(device_->GetScreenWidth(), 1)),
          static_cast<UINT>(std::max(device_->GetScreenHeight(), 1)))) {
    return false;
  }

  cube_model_ = std::make_shared<ReflectionModel>(device_);
  if (!cube_model_) {
    return false;
  }

  WCHAR *cube_textures[1] = {L"data/seafloor.dds"};
  if (!cube_model_->Initialize(L"data/cube.txt", cube_textures,
                               _countof(cube_textures))) {
    return false;
  }

  cube_material_ = std::make_shared<ReflectionTextureMaterial>(device_);
  if (!cube_material_) {
    return false;
  }

  cube_material_->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/textureVS.hlsl")
          .Get()));
  cube_material_->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/texturePS.hlsl")
          .Get()));
  if (!cube_material_->Initialize()) {
    return false;
  }

  floor_model_ = std::make_shared<ReflectionModel>(device_);
  if (!floor_model_) {
    return false;
  }

  WCHAR *floor_textures[1] = {L"data/blue01.dds"};
  if (!floor_model_->Initialize(L"data/floor.txt", floor_textures,
                                _countof(floor_textures))) {
    return false;
  }

  floor_material_ = std::make_shared<ReflectionFloorMaterial>(device_);
  if (!floor_material_) {
    return false;
  }

  floor_material_->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetVertexShaderBlobByFileName(L"shader/reflection.hlsl")
          .Get()));
  floor_material_->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
      shader_loader_->GetPixelShaderBlobByFileName(L"shader/reflection.hlsl")
          .Get()));
  if (!floor_material_->Initialize()) {
    return false;
  }

  if (!BuildFloorDescriptorHeap()) {
    return false;
  }

  return true;
}

auto ReflectionScene::Shutdown() -> void {
  cube_model_.reset();
  floor_model_.reset();
  cube_material_.reset();
  floor_material_.reset();
  render_texture_.reset();
  floor_descriptor_heap_.Reset();
  shaders_loaded_ = false;
}

auto ReflectionScene::Update(float delta_seconds) -> void {
  rotation_radians_ += rotation_speed_ * delta_seconds;
  if (rotation_radians_ > XM_2PI) {
    rotation_radians_ -= XM_2PI;
  }
}

auto ReflectionScene::RenderReflectionMap(const XMMATRIX &projection) -> bool {
  return RenderReflectionTexture(projection);
}

auto ReflectionScene::Render(const XMMATRIX &view,
                             const XMMATRIX &projection) -> bool {
  if (!device_ || !cube_model_ || !floor_model_ || !cube_material_ ||
      !floor_material_) {
    return false;
  }

  const XMMATRIX world = XMMatrixRotationY(rotation_radians_) *
                         XMMatrixTranslation(cube_position_.x, cube_position_.y,
                                             cube_position_.z);
  const XMMATRIX world_t = XMMatrixTranspose(world);
  const XMMATRIX view_t = XMMatrixTranspose(view);
  const XMMATRIX projection_t = XMMatrixTranspose(projection);

  if (!cube_material_->UpdateMatrixConstant(world_t, view_t, projection_t)) {
    return false;
  }

  auto cube_srv = cube_model_->GetShaderResourceView();
  auto cube_matrix_cb = cube_material_->GetMatrixConstantBuffer();
  if (!cube_srv || !cube_matrix_cb) {
    return false;
  }

  auto cube_root_signature = cube_material_->GetRootSignature();
  auto cube_pso = cube_material_->GetPSOByName("reflection_texture_main");
  if (!cube_root_signature || !cube_pso) {
    return false;
  }

  device_->SetGraphicsRootSignature(cube_root_signature);
  device_->SetPipelineStateObject(cube_pso);

  ID3D12DescriptorHeap *cube_heaps[] = {cube_srv.Get()};
  device_->SetDescriptorHeaps(1, cube_heaps);
  device_->SetGraphicsRootDescriptorTable(
      0, cube_srv->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, cube_matrix_cb->GetGPUVirtualAddress());

  device_->BindVertexBuffer(0, 1, &cube_model_->GetVertexBufferView());
  device_->BindIndexBuffer(&cube_model_->GetIndexBufferView());
  device_->Draw(cube_model_->GetIndexCount());

  const XMMATRIX floor_world =
      XMMatrixScaling(floor_scale_, 1.0f, floor_scale_) *
      XMMatrixTranslation(0.0f, reflection_plane_height_, 0.0f);
  const XMMATRIX floor_world_t = XMMatrixTranspose(floor_world);
  if (!floor_material_->UpdateMatrixConstant(floor_world_t, view_t,
                                             projection_t)) {
    return false;
  }

  camera_->UpdateReflection(reflection_plane_height_);
  XMMATRIX reflection_view;
  camera_->GetReflectionViewMatrix(reflection_view);
  const XMMATRIX reflection_t = XMMatrixTranspose(reflection_view);
  if (!floor_material_->UpdateReflectionConstant(reflection_t)) {
    return false;
  }

  auto floor_matrix_cb = floor_material_->GetMatrixConstantBuffer();
  auto floor_reflection_cb = floor_material_->GetReflectionConstantBuffer();
  if (!floor_descriptor_heap_ || !floor_matrix_cb || !floor_reflection_cb) {
    return false;
  }

  auto floor_root_signature = floor_material_->GetRootSignature();
  auto floor_pso = floor_material_->GetPSOByName("reflection_floor_main");
  if (!floor_root_signature || !floor_pso) {
    return false;
  }

  device_->SetGraphicsRootSignature(floor_root_signature);
  device_->SetPipelineStateObject(floor_pso);

  ID3D12DescriptorHeap *floor_heaps[] = {floor_descriptor_heap_.Get()};
  device_->SetDescriptorHeaps(1, floor_heaps);
  device_->SetGraphicsRootDescriptorTable(
      0, floor_descriptor_heap_->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, floor_matrix_cb->GetGPUVirtualAddress());
  device_->SetGraphicsRootConstantBufferView(
      2, floor_reflection_cb->GetGPUVirtualAddress());

  device_->BindVertexBuffer(0, 1, &floor_model_->GetVertexBufferView());
  device_->BindIndexBuffer(&floor_model_->GetIndexBufferView());
  device_->Draw(floor_model_->GetIndexCount());

  return true;
}

auto ReflectionScene::RenderReflectionTexture(const XMMATRIX &projection)
    -> bool {
  if (!render_texture_ || !cube_model_ || !cube_material_) {
    return false;
  }

  if (!render_texture_->BeginRender()) {
    return false;
  }

  camera_->UpdateReflection(reflection_plane_height_);

  XMMATRIX reflection_view;
  camera_->GetReflectionViewMatrix(reflection_view);

  const XMMATRIX world = XMMatrixRotationY(rotation_radians_) *
                         XMMatrixTranslation(cube_position_.x, cube_position_.y,
                                             cube_position_.z);
  const XMMATRIX world_t = XMMatrixTranspose(world);
  const XMMATRIX view_t = XMMatrixTranspose(reflection_view);
  const XMMATRIX projection_t = XMMatrixTranspose(projection);

  if (!cube_material_->UpdateMatrixConstant(world_t, view_t, projection_t)) {
    render_texture_->EndRender();
    return false;
  }

  auto cube_srv = cube_model_->GetShaderResourceView();
  auto cube_matrix_cb = cube_material_->GetMatrixConstantBuffer();
  if (!cube_srv || !cube_matrix_cb) {
    render_texture_->EndRender();
    return false;
  }

  auto root_signature = cube_material_->GetRootSignature();
  auto pso = cube_material_->GetPSOByName("reflection_texture_main");
  if (!root_signature || !pso) {
    render_texture_->EndRender();
    return false;
  }

  device_->SetGraphicsRootSignature(root_signature);
  device_->SetPipelineStateObject(pso);

  ID3D12DescriptorHeap *heaps[] = {cube_srv.Get()};
  device_->SetDescriptorHeaps(1, heaps);
  device_->SetGraphicsRootDescriptorTable(
      0, cube_srv->GetGPUDescriptorHandleForHeapStart());
  device_->SetGraphicsRootConstantBufferView(
      1, cube_matrix_cb->GetGPUVirtualAddress());

  device_->BindVertexBuffer(0, 1, &cube_model_->GetVertexBufferView());
  device_->BindIndexBuffer(&cube_model_->GetIndexBufferView());
  device_->Draw(cube_model_->GetIndexCount());

  render_texture_->EndRender();
  return true;
}

auto ReflectionScene::BuildFloorDescriptorHeap() -> bool {
  if (!device_ || !floor_model_ || !render_texture_) {
    return false;
  }

  auto base_texture = floor_model_->GetTextureResource(0);
  auto reflection_texture = render_texture_->GetTexture();
  if (!base_texture || !reflection_texture) {
    return false;
  }

  auto d3d_device = device_->GetD3d12Device();
  if (!d3d_device) {
    return false;
  }

  floor_descriptor_heap_.Reset();

  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.NumDescriptors = 2;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

  if (FAILED(d3d_device->CreateDescriptorHeap(&heap_desc,
                                              IID_PPV_ARGS(&floor_descriptor_heap_)))) {
    return false;
  }

  UINT increment_size =
      d3d_device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

  CD3DX12_CPU_DESCRIPTOR_HANDLE dest_handle(
      floor_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

  D3D12_SHADER_RESOURCE_VIEW_DESC base_srv_desc = {};
  auto base_desc = base_texture->GetDesc();
  base_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  base_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  base_srv_desc.Format = base_desc.Format;
  base_srv_desc.Texture2D.MipLevels = base_desc.MipLevels;
  base_srv_desc.Texture2D.MostDetailedMip = 0;
  base_srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  d3d_device->CreateShaderResourceView(base_texture.Get(), &base_srv_desc,
                                       dest_handle);

  dest_handle.Offset(1, increment_size);

  D3D12_SHADER_RESOURCE_VIEW_DESC reflection_srv_desc = {};
  auto reflection_desc = reflection_texture->GetDesc();
  reflection_srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  reflection_srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  reflection_srv_desc.Format = reflection_desc.Format;
  reflection_srv_desc.Texture2D.MipLevels = reflection_desc.MipLevels;
  reflection_srv_desc.Texture2D.MostDetailedMip = 0;
  reflection_srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

  d3d_device->CreateShaderResourceView(reflection_texture.Get(),
                                       &reflection_srv_desc, dest_handle);

  return true;
}

