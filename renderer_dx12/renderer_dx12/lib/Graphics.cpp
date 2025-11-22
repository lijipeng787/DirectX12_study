#include "stdafx.h"

#include "Graphics.h"

#include "BumpMappingScene.h"
#include "CPUUsageTracker.h"
#include "Camera.h"
#include "DirectX12Device.h"
#include "Fps.h"
#include "Input.h"
#include "LegacyModelScene.h"
#include "LightManager.h"
#include "PBRModelScene.h"
#include "ReflectionScene.h"
#include "Scene.h"
#include "SpecularMappingScene.h"
#include "Text.h"
#include "TextOverlayScene.h"

bool Graphics::Initialize(int screenWidth, int screenHeight, HWND hwnd) {
  // Initialize DirectX12 Device
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

  // Initialize system components
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

  // Initialize shaders
  if (!InitializeShaders(hwnd)) {
    return false;
  }

  // Initialize render objects
  if (!InitializeRenderObjects(screenWidth, screenHeight, hwnd)) {
    return false;
  }

  // Initialize scenes
  if (!InitializeScenes(hwnd)) {
    return false;
  }

  return true;
}

void Graphics::Shutdown() {

  if (d3d12_device_) {
    d3d12_device_->WaitForGpuIdle();
  }

  // Shutdown all scenes
  for (auto& scene : scenes_) {
    scene.Shutdown();
  }
  scenes_.clear();

  text_cache_.reset();
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
  if (text_cache_) {
    if (!text_cache_->SetCpu(cpu_usage_tracker_->GetCpuPercentage())) {
      return false;
    }
  }

  fps_->Frame();
  if (text_cache_) {
    if (!text_cache_->SetFps(fps_->GetFps())) {
      return false;
    }
  }

  // Update all scenes
  SceneUpdateContext update_ctx{delta_seconds};
  for (auto& scene : scenes_) {
    scene.Update(update_ctx);
  }

  // Update shared rotation angle
  shared_rotation_angle_ += shared_rotation_speed_ * delta_seconds;
  if (shared_rotation_angle_ > DirectX::XM_2PI) {
    shared_rotation_angle_ -= DirectX::XM_2PI;
  }

  // Set rotation for all scenes
  for (auto& scene : scenes_) {
    scene.SetRotationAngle(shared_rotation_angle_);
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
  // Get view and projection matrices
  DirectX::XMMATRIX view_matrix = camera_->GetViewMatrix();
  DirectX::XMMATRIX projection_matrix = {};
  d3d12_device_->GetProjectionMatrix(projection_matrix);

  // Reset command allocator and command list
  if (!d3d12_device_->ResetCommandAllocator() || 
      !d3d12_device_->ResetCommandList()) {
    return false;
  }

  // Render reflection pre-pass (generates reflection textures)
  SceneReflectionContext refl_ctx{projection_matrix};
  for (auto& scene : scenes_) {
    if (!scene.RenderReflection(refl_ctx)) {
      return false;
    }
  }

  // Begin main rendering pass
  d3d12_device_->BeginPopulateGraphicsCommandList();

  // Get main light for scenes
  auto main_light = light_manager_->GetPrimaryLight();
  if (!main_light) {
    return false;
  }

  // Render all scenes with unified interface
  SceneRenderContext render_ctx{view_matrix, projection_matrix, main_light.get()};
  for (auto& scene : scenes_) {
    if (!scene.Render(render_ctx)) {
      return false;
    }
  }

  d3d12_device_->EndPopulateGraphicsCommandList();

  // Execute and present
  if (!d3d12_device_->ExecuteDefaultGraphicsCommandList()) {
    return false;
  }

  return true;
}

bool Graphics::InitializeShaders(HWND hwnd) {
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

  // Compile texture shaders
  ShaderCompileDesc texture_vs{L"shader/texture.hlsl",
                               "TextureVertexShader", "vs_5_0"};
  ShaderCompileDesc texture_ps{L"shader/texture.hlsl",
                               "TexturePixelShader", "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(texture_vs, texture_ps)) {
    report_shader_error(L"Could not initialize Texture Shader.");
    return false;
  }

  // Compile light shaders
  ShaderCompileDesc light_vs{L"shader/light.hlsl", "LightVertexShader",
                             "vs_5_0"};
  ShaderCompileDesc light_ps{L"shader/light.hlsl", "LightPixelShader",
                             "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(light_vs, light_ps)) {
    report_shader_error(L"Could not initialize Light Shader.");
    return false;
  }

  // Compile font shaders
  ShaderCompileDesc font_vs{L"shader/font.hlsl", "FontVertexShader",
                            "vs_5_0"};
  ShaderCompileDesc font_ps{L"shader/font.hlsl", "FontPixelShader",
                            "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(font_vs, font_ps)) {
    report_shader_error(L"Could not initialize Font Shader.");
    return false;
  }

  // Compile PBR shaders
  ShaderCompileDesc pbr_vs{L"shader/pbr.hlsl", "PbrVertexShader",
                           "vs_5_0"};
  ShaderCompileDesc pbr_ps{L"shader/pbr.hlsl", "PbrPixelShader",
                           "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(pbr_vs, pbr_ps)) {
    report_shader_error(L"Could not initialize PBR Shader.");
    return false;
  }

  // Compile specular mapping shaders
  ShaderCompileDesc spec_vs{L"shader/specMap.hlsl", "SpecMapVertexShader",
                            "vs_5_0"};
  ShaderCompileDesc spec_ps{L"shader/specMap.hlsl", "SpecMapPixelShader",
                            "ps_5_0"};
  if (!shader_loader_->CompileVertexAndPixelShaders(spec_vs, spec_ps)) {
    report_shader_error(L"Could not initialize Specular Map Shader.");
    return false;
  }

  return true;
}

bool Graphics::InitializeRenderObjects(int /*screenWidth*/, int /*screenHeight*/, HWND /*hwnd*/) {
  // All render objects now managed as scenes
  return true;
}

bool Graphics::InitializeScenes(HWND hwnd) {
  // Prepare scene initialization context
  SceneInitializeContext scene_ctx{};
  scene_ctx.device = d3d12_device_;
  scene_ctx.shader_loader = shader_loader_;
  scene_ctx.light_manager = light_manager_;
  scene_ctx.camera = camera_;
  scene_ctx.hwnd = hwnd;

  // Create legacy model scene (textured cube with fog)
  auto legacy_scene = std::make_shared<LegacyModelScene>();
  if (!legacy_scene || !legacy_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize legacy model scene.", L"Error", MB_OK);
    return false;
  }
  scenes_.emplace_back(std::move(legacy_scene));

  // Create PBR model scene
  auto pbr_scene = std::make_shared<PBRModelScene>();
  if (!pbr_scene || !pbr_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize PBR scene.", L"Error", MB_OK);
    return false;
  }
  scenes_.emplace_back(std::move(pbr_scene));

  // Create bump mapping scene
  auto bump_scene = std::make_shared<BumpMappingScene>();
  if (!bump_scene || !bump_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize bump mapping scene.", L"Error", MB_OK);
    return false;
  }
  scenes_.emplace_back(std::move(bump_scene));

  // Create specular mapping scene
  auto specular_scene = std::make_shared<SpecularMappingScene>();
  if (!specular_scene || !specular_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize specular mapping scene.", L"Error", MB_OK);
    return false;
  }
  scenes_.emplace_back(std::move(specular_scene));

  // Create reflection scene
  auto reflection_scene = std::make_shared<ReflectionScene>();
  if (!reflection_scene || !reflection_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize reflection scene.", L"Error", MB_OK);
    return false;
  }
  scenes_.emplace_back(std::move(reflection_scene));

  // Create text overlay scene
  auto text_scene = std::make_shared<TextOverlayScene>();
  if (!text_scene || !text_scene->Initialize(scene_ctx)) {
    MessageBox(hwnd, L"Could not initialize text overlay scene.", L"Error", MB_OK);
    return false;
  }
  text_cache_ = text_scene->GetText();  // Cache for SetFps/SetCpu calls
  scenes_.emplace_back(std::move(text_scene));

  return true;
}
