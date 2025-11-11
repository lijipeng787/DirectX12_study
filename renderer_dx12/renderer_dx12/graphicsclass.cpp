#include "stdafx.h"

#include "graphicsclass.h"

#include "BumpMappingScene.h"
#include "CPUUsageTracker.h"
#include "Camera.h"
#include "DirectX12Device.h"
#include "Fps.h"
#include "Input.h"
#include "LightManager.h"
#include "PBRModel.h"
#include "SpecularMappingScene.h"
#include "ReflectionScene.h"

#include "ScreenQuad.h"
#include "modelclass.h"
#include "textclass.h"

bool Graphics::Initialize(int screenWidth, int screenHeight, HWND hwnd) {

  DirectX12DeviceConfig device_config = {};
  device_config.screen_width = screenWidth;
  device_config.screen_height = screenHeight;
  device_config.vsync_enabled = VSYNC_ENABLED;
  device_config.hwnd = hwnd;
  device_config.fullscreen = FULL_SCREEN;
  device_config.screen_depth = SCREEN_DEPTH;
  device_config.screen_near = SCREEN_NEAR;

  d3d12_device_ = DirectX12Device::Create(device_config);
  if (!d3d12_device_) {
    MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
    return false;
  }

  cpu_usage_tracker_ = std::make_shared<CPUUsageTracker>();
  if (!cpu_usage_tracker_) {
    return false;
  }
  cpu_usage_tracker_->Initialize();

  fps_ = std::make_shared<Fps>();
  if (!fps_) {
    return false;
  }

  camera_ = std::make_shared<Camera>();
  if (!camera_) {
    return false;
  }
  camera_->SetPosition(0.0f, 0.0f, -5.0f);
  camera_->Update();

  // Initialize unified light manager
  light_manager_ = std::make_shared<Lighting::LightManager>();
  if (!light_manager_) {
    return false;
  }

  // Create the main directional light
  auto main_light = light_manager_->CreateLight(
      "MainLight", Lighting::LightType::Directional);
  if (!main_light) {
    return false;
  }

  // Set light properties (same as old Light class settings)
  main_light->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
  main_light->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
  main_light->SetDirection(0.0f, 0.0f, 1.0f);
  main_light->SetColor(1.0f, 1.0f, 1.0f); // White light
  main_light->SetIntensity(1.0f);

  {
    shader_loader_ = std::make_shared<ResourceLoader::ShaderLoader>();
    if (!shader_loader_) {
      return false;
    }

    using ResourceLoader::ShaderCompileDesc;

    const auto report_shader_error = [this, hwnd](const wchar_t *fallback) {
      const auto &error = shader_loader_->GetLastErrorMessage();
      if (!error.empty()) {
        MessageBoxA(hwnd, error.c_str(), "Shader Compilation Error", MB_OK);
      } else if (fallback != nullptr) {
        MessageBox(hwnd, fallback, L"Error", MB_OK);
      }
    };

    ShaderCompileDesc texture_vs{L"shader/textureVS.hlsl",
                                 "TextureVertexShader", "vs_5_0"};
    ShaderCompileDesc texture_ps{L"shader/texturePS.hlsl",
                                 "TexturePixelShader", "ps_5_0"};
    if (!shader_loader_->CompileVertexAndPixelShaders(texture_vs,
                                                      texture_ps)) {
      report_shader_error(L"Could not initialize Texture Shader.");
      return false;
    }

    ShaderCompileDesc light_vs{L"shader/lightVS.hlsl", "LightVertexShader",
                               "vs_5_0"};
    ShaderCompileDesc light_ps{L"shader/lightPS.hlsl", "LightPixelShader",
                               "ps_5_0"};
    if (!shader_loader_->CompileVertexAndPixelShaders(light_vs, light_ps)) {
      report_shader_error(L"Could not initialize Light Shader.");
      return false;
    }

    ShaderCompileDesc font_vs{L"shader/fontVS.hlsl", "FontVertexShader",
                              "vs_5_0"};
    ShaderCompileDesc font_ps{L"shader/fontPS.hlsl", "FontPixelShader",
                              "ps_5_0"};
    if (!shader_loader_->CompileVertexAndPixelShaders(font_vs, font_ps)) {
      report_shader_error(L"Could not initialize Font Shader.");
      return false;
    }

    ShaderCompileDesc pbr_vs{L"shader/pbrVS.hlsl", "PbrVertexShader",
                             "vs_5_0"};
    ShaderCompileDesc pbr_ps{L"shader/pbrPS.hlsl", "PbrPixelShader",
                             "ps_5_0"};
    if (!shader_loader_->CompileVertexAndPixelShaders(pbr_vs, pbr_ps)) {
      report_shader_error(L"Could not initialize PBR Shader.");
      return false;
    }

    ShaderCompileDesc spec_vs{L"shader/specMap.hlsl", "SpecMapVertexShader",
                              "vs_5_0"};
    ShaderCompileDesc spec_ps{L"shader/specMap.hlsl", "SpecMapPixelShader",
                              "ps_5_0"};
    if (!shader_loader_->CompileVertexAndPixelShaders(spec_vs, spec_ps)) {
      report_shader_error(L"Could not initialize Specular Map Shader.");
      return false;
    }
  }

  {
    auto bitmap_material = std::make_shared<ScreenQuadMaterial>(d3d12_device_);
    if (!bitmap_material) {
      return false;
    }
    bitmap_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetVertexShaderBlobByFileName(L"shader/textureVS.hlsl")
            .Get()));
    bitmap_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"shader/texturePS.hlsl")
            .Get()));

    bitmap_ = std::make_shared<ScreenQuad>(d3d12_device_, bitmap_material);
    if (!bitmap_) {
      return false;
    }
    if (!bitmap_->Initialize(screenWidth, screenHeight, 255, 255)) {
      MessageBox(hwnd, L"Could not initialize Bitmap.", L"Error", MB_OK);
      return false;
    }
  }

  {
    model_ = std::make_shared<Model>(d3d12_device_);
    if (!model_) {
      return false;
    }
    ModelMaterial *model_material = model_->GetMaterial();
    model_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetVertexShaderBlobByFileName(L"shader/lightVS.hlsl")
            .Get()));
    model_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"shader/lightPS.hlsl")
            .Get()));

    WCHAR *texture_filename_arr[3] = {L"data/stone01.dds", L"data/dirt01.dds",
                                      L"data/alpha01.dds"};
    if (!model_->Initialize(L"data/cube.txt", texture_filename_arr)) {
      MessageBox(hwnd, L"Could not initialize Model.", L"Error", MB_OK);
      return false;
    }
  }

  {
    text_ = std::make_shared<Text>(d3d12_device_);
    if (!text_) {
      return false;
    }
    TextMaterial *text_material = text_->GetMaterial();
    text_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetVertexShaderBlobByFileName(L"shader/fontVS.hlsl")
            .Get()));
    text_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"shader/fontPS.hlsl")
            .Get()));

    WCHAR *font_texture[1] = {L"data/font.dds"};
    if (!text_->LoadFont(L"data/fontdata.txt", font_texture)) {
      MessageBox(hwnd, L"Could not initialize Font data.", L"Error", MB_OK);
      return false;
    }

    DirectX::XMMATRIX base_matrix = {};
    camera_->GetViewMatrix(base_matrix);
    if (!text_->Initialize(screenWidth, screenHeight, base_matrix)) {
      return false;
    }
  }

  {
    pbr_model_ = std::make_shared<PBRModel>(d3d12_device_);
    if (!pbr_model_) {
      return false;
    }
    PBRMaterial *pbr_material = pbr_model_->GetMaterial();
    pbr_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetVertexShaderBlobByFileName(L"shader/pbrVS.hlsl")
            .Get()));
    pbr_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"shader/pbrPS.hlsl")
            .Get()));

    WCHAR *pbr_textures[3] = {L"data/pbr/pbr_albedo.tga",
                              L"data/pbr/pbr_normal.tga",
                              L"data/pbr/pbr_roughmetal.tga"};
    if (!pbr_model_->Initialize(L"data/pbr/sphere.txt", pbr_textures)) {
      MessageBox(hwnd, L"Could not initialize PBR Model.", L"Error", MB_OK);
      return false;
    }
  }

  {
    bump_mapping_scene_ = std::make_shared<BumpMappingScene>(
        d3d12_device_, shader_loader_, light_manager_, camera_);
    if (!bump_mapping_scene_) {
      return false;
    }

    if (!bump_mapping_scene_->Initialize()) {
      MessageBox(hwnd, L"Could not initialize bump mapping scene.", L"Error",
                 MB_OK);
      return false;
    }
  }

  {
    specular_mapping_scene_ = std::make_shared<SpecularMappingScene>(
        d3d12_device_, shader_loader_, light_manager_, camera_);
    if (!specular_mapping_scene_) {
      return false;
    }

    if (!specular_mapping_scene_->Initialize()) {
      MessageBox(hwnd, L"Could not initialize specular mapping scene.", L"Error",
                 MB_OK);
      return false;
    }
  }

  {
    reflection_scene_ = std::make_shared<ReflectionScene>(
        d3d12_device_, shader_loader_, camera_);
    if (!reflection_scene_) {
      return false;
    }

    if (!reflection_scene_->Initialize()) {
      MessageBox(hwnd, L"Could not initialize reflection scene.", L"Error",
                 MB_OK);
      return false;
    }
  }

  return true;
}

void Graphics::Shutdown() {

  if (d3d12_device_) {
    d3d12_device_->WaitForGpuIdle();
  }

  if (bump_mapping_scene_) {
    bump_mapping_scene_->Shutdown();
    bump_mapping_scene_.reset();
  }

  if (specular_mapping_scene_) {
    specular_mapping_scene_->Shutdown();
    specular_mapping_scene_.reset();
  }

  if (reflection_scene_) {
    reflection_scene_->Shutdown();
    reflection_scene_.reset();
  }

  bitmap_.reset();
  text_.reset();
  model_.reset();
  pbr_model_.reset();

  shader_loader_.reset();
  light_manager_.reset();
  camera_.reset();
  fps_.reset();

  if (cpu_usage_tracker_) {
    cpu_usage_tracker_->Shutdown();
    cpu_usage_tracker_.reset();
  }

  d3d12_device_.reset();
}

bool Graphics::Frame(float delta_seconds, Input *input) {

  cpu_usage_tracker_->Update();
  if (!text_->SetCpu(cpu_usage_tracker_->GetCpuPercentage())) {
    return false;
  }

  fps_->Frame();
  if (!text_->SetFps(fps_->GetFps())) {
    return false;
  }

  if (bump_mapping_scene_) {
    bump_mapping_scene_->Update(delta_seconds);
  }

  if (specular_mapping_scene_) {
    specular_mapping_scene_->Update(delta_seconds);
  }

  if (reflection_scene_) {
    reflection_scene_->Update(delta_seconds);
  }

  shared_rotation_angle_ += shared_rotation_speed_ * delta_seconds;
  if (shared_rotation_angle_ > DirectX::XM_2PI) {
    shared_rotation_angle_ -= DirectX::XM_2PI;
  }

  if (bump_mapping_scene_) {
    bump_mapping_scene_->SetRotationAngle(shared_rotation_angle_);
  }

  if (specular_mapping_scene_) {
    specular_mapping_scene_->SetRotationAngle(shared_rotation_angle_);
  }

  if (reflection_scene_) {
    reflection_scene_->SetRotationAngle(shared_rotation_angle_);
  }

  UpdateCameraFromInput(delta_seconds, input);

  camera_->Update();
  if (FAILED(Render())) {
    return false;
  }

  return true;
}

void Graphics::UpdateCameraFromInput(float delta_seconds, Input *input) {
  if (!camera_ || !input) {
    return;
  }

  if (delta_seconds <= 0.0f) {
    return;
  }

  using namespace DirectX;

  XMFLOAT3 camera_position = camera_->GetPosition();
  XMFLOAT3 camera_rotation = camera_->GetRotation();

  float rotation_delta = 0.0f;
  if (input->IsQPressed()) {
    rotation_delta -= camera_turn_speed_ * delta_seconds;
  }
  if (input->IsEPressed()) {
    rotation_delta += camera_turn_speed_ * delta_seconds;
  }

  bool rotation_changed = false;
  if (rotation_delta != 0.0f) {
    camera_rotation.y += rotation_delta;

    if (camera_rotation.y > 180.0f) {
      camera_rotation.y -= 360.0f;
    } else if (camera_rotation.y < -180.0f) {
      camera_rotation.y += 360.0f;
    }

    rotation_changed = true;
  }

  float yaw_radians = camera_rotation.y * XM_PI / 180.0f;

  XMVECTOR forward = XMVector3Normalize(
      XMVectorSet(sinf(yaw_radians), 0.0f, cosf(yaw_radians), 0.0f));
  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
  XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

  XMVECTOR movement = XMVectorZero();

  if (input->IsWPressed()) {
    movement = XMVectorAdd(movement, forward);
  }
  if (input->IsSPressed()) {
    movement = XMVectorSubtract(movement, forward);
  }
  if (input->IsDPressed()) {
    movement = XMVectorAdd(movement, right);
  }
  if (input->IsAPressed()) {
    movement = XMVectorSubtract(movement, right);
  }
  if (input->IsRPressed()) {
    movement = XMVectorAdd(movement, up);
  }
  if (input->IsFPressed()) {
    movement = XMVectorSubtract(movement, up);
  }

  float movement_length_sq = XMVectorGetX(XMVector3LengthSq(movement));
  if (movement_length_sq <= 0.0f) {
    if (rotation_changed) {
      camera_->SetRotation(camera_rotation.x, camera_rotation.y,
                           camera_rotation.z);
    }
    return;
  }

  movement = XMVector3Normalize(movement);

  float move_distance = camera_move_speed_ * delta_seconds;
  XMVECTOR position = XMLoadFloat3(&camera_position);
  position = XMVectorAdd(position, XMVectorScale(movement, move_distance));

  XMStoreFloat3(&camera_position, position);
  camera_->SetPosition(camera_position.x, camera_position.y, camera_position.z);
  if (rotation_changed) {
    camera_->SetRotation(camera_rotation.x, camera_rotation.y,
                         camera_rotation.z);
  }
}

bool Graphics::Render() {

  DirectX::XMMATRIX world_matrix = {};
  d3d12_device_->GetWorldMatrix(world_matrix);

  float rotation = shared_rotation_angle_;

  DirectX::XMMATRIX rotate_world =
      DirectX::XMMatrixTranspose(DirectX::XMMatrixRotationY(rotation) *
                                 DirectX::XMMatrixTranslation(-6.0f, 1.5f, -6.0f));

  DirectX::XMMATRIX font_world = DirectX::XMMatrixTranspose(world_matrix);

  DirectX::XMMATRIX projection_matrix = {};
  d3d12_device_->GetProjectionMatrix(projection_matrix);
  DirectX::XMMATRIX projection =
      DirectX::XMMatrixTranspose(projection_matrix);

  DirectX::XMMATRIX view_matrix = {};
  camera_->GetViewMatrix(view_matrix);
  DirectX::XMMATRIX view = DirectX::XMMatrixTranspose(view_matrix);

  DirectX::XMMATRIX orthogonality = {};
  d3d12_device_->GetOrthoMatrix(orthogonality);
  orthogonality = DirectX::XMMatrixTranspose(orthogonality);

  DirectX::XMMATRIX pbr_world = DirectX::XMMatrixRotationY(rotation) *
                                DirectX::XMMatrixTranslation(6.0f, 1.5f, -6.0f);
  pbr_world = DirectX::XMMatrixTranspose(pbr_world);

  if (!model_->GetMaterial()->UpdateMatrixConstant(rotate_world, view,
                                                        projection)) {
    return false;
  }

  // Use unified light system - get primary light from manager
  auto main_light = light_manager_->GetPrimaryLight();
  if (!main_light) {
    return false;
  }

  if (!model_->GetMaterial()->UpdateFromLight(main_light.get())) {
    return false;
  }

  if (!model_->GetMaterial()->UpdateFogConstant(3.0f, 6.0f)) {
    return false;
  }

  if (!text_->GetMaterial()->UpdateMatrixConstant(font_world, view,
                                                       orthogonality)) {
    return false;
  }

  DirectX::XMFLOAT4 pixel_color(1.0f, 0.0f, 0.0f, 0.0f);
  if (!text_->GetMaterial()->UpdateLightConstant(pixel_color)) {
    return false;
  }

  if (pbr_model_) {
    auto camera_position = camera_->GetPosition();
    auto pbr_material = pbr_model_->GetMaterial();
    if (!pbr_material->UpdateMatrixConstant(pbr_world, view, projection)) {
      return false;
    }
    if (!pbr_material->UpdateCameraConstant(camera_position)) {
      return false;
    }

    // Use unified light system for PBR as well
    if (!pbr_material->UpdateFromLight(main_light.get())) {
      return false;
    }
  }

  auto off_screen_root_signature = bitmap_->GetMaterial()->GetRootSignature();
  auto off_screen_pso = bitmap_->GetMaterial()->GetPSOByName("bitmap_normal");

  auto font_root_signature = text_->GetMaterial()->GetRootSignature();
  auto blend_enabled_pso =
      text_->GetMaterial()->GetPSOByName("text_blend_enable");
  auto font_matrix_constant = text_->GetMaterial()->GetMatrixConstantBuffer();
  auto font_pixel_constant = text_->GetMaterial()->GetPixelConstantBuffer();

  auto light_root_signature = model_->GetMaterial()->GetRootSignature();
  auto pso = model_->GetMaterial()->GetPSOByName("model_normal");
  auto light_matrix_constant = model_->GetMaterial()->GetMatrixConstantBuffer();
  auto light_constant = model_->GetMaterial()->GetLightConstantBuffer();
  auto fog_constant = model_->GetMaterial()->GetFogConstantBuffer();

  if (!d3d12_device_->ResetCommandAllocator()) {
    return false;
  }
  if (!d3d12_device_->ResetCommandList()) {
    return false;
  }

  {
    d3d12_device_->BeginDrawToOffScreen();

    d3d12_device_->SetGraphicsRootSignature(light_root_signature);
    d3d12_device_->SetPipelineStateObject(pso);

    ID3D12DescriptorHeap *light_shader_heap[] = {
        model_.get()->GetShaderRescourceView().Get()};
    d3d12_device_->SetDescriptorHeaps(1, light_shader_heap);

    d3d12_device_->SetGraphicsRootDescriptorTable(
        0, light_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        1, light_matrix_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        2, light_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        3, fog_constant->GetGPUVirtualAddress());

    d3d12_device_->BindIndexBuffer(&model_->GetIndexBufferView());
    d3d12_device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());

    d3d12_device_->Draw(model_->GetIndexCount());

    d3d12_device_->SetGraphicsRootSignature(font_root_signature);
    d3d12_device_->SetPipelineStateObject(blend_enabled_pso);

    ID3D12DescriptorHeap *font_shader_heap[] = {
        text_.get()->GetShaderRescourceView().Get()};
    d3d12_device_->SetDescriptorHeaps(1, font_shader_heap);

    d3d12_device_->SetGraphicsRootDescriptorTable(
        0, font_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        1, font_matrix_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        2, font_pixel_constant->GetGPUVirtualAddress());

    auto vertex1 = text_->GetVertexBufferView(0);
    auto vertex2 = text_->GetVertexBufferView(1);

    auto index1 = text_->GetIndexBufferView(0);
    auto index2 = text_->GetIndexBufferView(1);

    d3d12_device_->BindIndexBuffer(&index1);

    d3d12_device_->BindVertexBuffer(0, 1, &vertex1);

    d3d12_device_->Draw(text_->GetIndexCount(0));

    d3d12_device_->BindIndexBuffer(&index2);

    d3d12_device_->BindVertexBuffer(0, 1, &vertex2);

    d3d12_device_->Draw(text_->GetIndexCount(1));

    d3d12_device_->EndDrawToOffScreen();
  }

  if (reflection_scene_) {
    if (!reflection_scene_->RenderReflectionMap(projection_matrix)) {
      return false;
    }
  }

  {
    d3d12_device_->BeginPopulateGraphicsCommandList();

    if (reflection_scene_) {
      if (!reflection_scene_->Render(view_matrix, projection_matrix)) {
        return false;
      }
    }

    if (specular_mapping_scene_) {
      if (!specular_mapping_scene_->Render(view_matrix, projection_matrix,
                                           main_light.get())) {
        return false;
      }
    }

    if (bump_mapping_scene_) {
      if (!bump_mapping_scene_->Render(view_matrix, projection_matrix,
                                       main_light.get())) {
        return false;
      }
    }

    if (pbr_model_) {
      auto pbr_material = pbr_model_->GetMaterial();
      auto pbr_root_signature = pbr_material->GetRootSignature();
      auto pbr_pso = pbr_material->GetPSOByName("pbr_pipeline");
      auto pbr_matrix_cb = pbr_material->GetMatrixConstantBuffer();
      auto pbr_camera_cb = pbr_material->GetCameraConstantBuffer();
      auto pbr_light_cb = pbr_material->GetLightConstantBuffer();

      ID3D12DescriptorHeap *pbr_heap[] = {
          pbr_model_->GetShaderRescourceView().Get()};
      d3d12_device_->SetDescriptorHeaps(1, pbr_heap);

      d3d12_device_->SetGraphicsRootSignature(pbr_root_signature);
      d3d12_device_->SetPipelineStateObject(pbr_pso);
      d3d12_device_->SetGraphicsRootDescriptorTable(
          0, pbr_heap[0]->GetGPUDescriptorHandleForHeapStart());
      d3d12_device_->SetGraphicsRootConstantBufferView(
          1, pbr_matrix_cb->GetGPUVirtualAddress());
      d3d12_device_->SetGraphicsRootConstantBufferView(
          2, pbr_camera_cb->GetGPUVirtualAddress());
      d3d12_device_->SetGraphicsRootConstantBufferView(
          3, pbr_light_cb->GetGPUVirtualAddress());

      d3d12_device_->BindVertexBuffer(0, 1, &pbr_model_->GetVertexBufferView());
      d3d12_device_->BindIndexBuffer(&pbr_model_->GetIndexBufferView());
      d3d12_device_->Draw(pbr_model_->GetIndexCount());
    }

    d3d12_device_->SetGraphicsRootSignature(light_root_signature);
    d3d12_device_->SetPipelineStateObject(pso);

    ID3D12DescriptorHeap *light_shader_heap[] = {
        model_.get()->GetShaderRescourceView().Get()};
    d3d12_device_->SetDescriptorHeaps(1, light_shader_heap);

    d3d12_device_->SetGraphicsRootDescriptorTable(
        0, light_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        1, light_matrix_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        2, light_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        3, fog_constant->GetGPUVirtualAddress());

    d3d12_device_->BindIndexBuffer(&model_->GetIndexBufferView());
    d3d12_device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());

    d3d12_device_->Draw(model_->GetIndexCount());

    d3d12_device_->SetGraphicsRootSignature(font_root_signature);
    d3d12_device_->SetPipelineStateObject(blend_enabled_pso);

    ID3D12DescriptorHeap *font_shader_heap[] = {
        text_.get()->GetShaderRescourceView().Get()};
    d3d12_device_->SetDescriptorHeaps(1, font_shader_heap);

    d3d12_device_->SetGraphicsRootDescriptorTable(
        0, font_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        1, font_matrix_constant->GetGPUVirtualAddress());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        2, font_pixel_constant->GetGPUVirtualAddress());

    auto vertex1 = text_->GetVertexBufferView(0);
    auto vertex2 = text_->GetVertexBufferView(1);

    auto index1 = text_->GetIndexBufferView(0);
    auto index2 = text_->GetIndexBufferView(1);

    d3d12_device_->BindIndexBuffer(&index1);

    d3d12_device_->BindVertexBuffer(0, 1, &vertex1);

    d3d12_device_->Draw(text_->GetIndexCount(0));

    d3d12_device_->BindIndexBuffer(&index2);

    d3d12_device_->BindVertexBuffer(0, 1, &vertex2);

    d3d12_device_->Draw(text_->GetIndexCount(1));

    d3d12_device_->BindVertexBuffer(0, 1, &bitmap_->GetVertexBufferView());
    d3d12_device_->BindIndexBuffer(&bitmap_->GetIndexBufferView());

    d3d12_device_->SetGraphicsRootSignature(off_screen_root_signature);
    d3d12_device_->SetPipelineStateObject(off_screen_pso);

    bitmap_->GetMaterial()->UpdateConstantBuffer(font_world, view,
                                                 orthogonality);
    bitmap_->UpdatePosition(100, 100);

    auto off_screen_heap = d3d12_device_->GetOffScreenTextureHeapView();
    ID3D12DescriptorHeap *off_screen_descriptor_heap[] = {
        off_screen_heap.Get()};
    d3d12_device_->SetDescriptorHeaps(1, off_screen_descriptor_heap);
    d3d12_device_->SetGraphicsRootDescriptorTable(
        0, off_screen_descriptor_heap[0]->GetGPUDescriptorHandleForHeapStart());
    d3d12_device_->SetGraphicsRootConstantBufferView(
        1, bitmap_->GetMaterial()->GetConstantBuffer()->GetGPUVirtualAddress());

    d3d12_device_->Draw(bitmap_->GetIndexCount());

    d3d12_device_->EndPopulateGraphicsCommandList();
  }

  if (!d3d12_device_->ExecuteDefaultGraphicsCommandList()) {
    return false;
  }

  return true;
}
