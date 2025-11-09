#include "stdafx.h"

#include "graphicsclass.h"

#include "CPUUsageTracker.h"
#include "Camera.h"
#include "DirectX12Device.h"
#include "Fps.h"
#include "Input.h"
#include "Light.h"
#include "PBRModel.h"

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

  light_ = std::make_shared<Light>();
  if (!light_) {
    return false;
  }
  light_->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
  light_->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
  light_->SetDirection(0.0f, 0.0f, 1.0f);

  // Initialize PBR light direction (normalized)
  pbr_light_direction_ = DirectX::XMFLOAT3(0.0f, 0.0f, 1.0f);

  {
    shader_loader_ = std::make_shared<ResourceLoader::ShaderLoader>();
    if (!shader_loader_) {
      return false;
    }

    shader_loader_->SetVSEntryPoint("TextureVertexShader");
    shader_loader_->SetPSEntryPoint("TexturePixelShader");
    if (CHECK(shader_loader_->CreateVSAndPSFromFile(L"textureVS.hlsl",
                                                    L"texturePS.hlsl"))) {
      MessageBox(hwnd, L"Could not initialize Texture Shader.", L"Error",
                 MB_OK);
      return false;
    }

    shader_loader_->SetVSEntryPoint("LightVertexShader");
    shader_loader_->SetPSEntryPoint("LightPixelShader");
    if (CHECK(shader_loader_->CreateVSAndPSFromFile(L"lightVS.hlsl",
                                                    L"lightPS.hlsl"))) {
      MessageBox(hwnd, L"Could not initialize Light Shader.", L"Error", MB_OK);
      return false;
    }

    shader_loader_->SetVSEntryPoint("FontVertexShader");
    shader_loader_->SetPSEntryPoint("FontPixelShader");
    if (CHECK(shader_loader_->CreateVSAndPSFromFile(L"fontVS.hlsl",
                                                    L"fontPS.hlsl"))) {
      MessageBox(hwnd, L"Could not initialize Font Shader.", L"Error", MB_OK);
      return false;
    }

    shader_loader_->SetVSEntryPoint("PbrVertexShader");
    shader_loader_->SetPSEntryPoint("PbrPixelShader");
    if (CHECK(shader_loader_->CreateVSAndPSFromFile(L"pbrVS.hlsl",
                                                    L"pbrPS.hlsl"))) {
      MessageBox(hwnd, L"Could not initialize PBR Shader.", L"Error", MB_OK);
      return false;
    }
  }

  {
    auto bitmap_material = std::make_shared<ScreenQuadMaterial>(d3d12_device_);
    if (!bitmap_material) {
      return false;
    }
    bitmap_material->SetVSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetVertexShaderBlobByFileName(L"textureVS.hlsl")
            .Get()));
    bitmap_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"texturePS.hlsl").Get()));

    bitmap_ = std::make_shared<ScreenQuad>(d3d12_device_, bitmap_material);
    if (!bitmap_) {
      return false;
    }
    if (CHECK(bitmap_->Initialize(screenWidth, screenHeight, 255, 255))) {
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
        shader_loader_->GetVertexShaderBlobByFileName(L"lightVS.hlsl").Get()));
    model_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"lightPS.hlsl").Get()));

    WCHAR *texture_filename_arr[3] = {L"data/stone01.dds", L"data/dirt01.dds",
                                      L"data/alpha01.dds"};
    if (CHECK(model_->Initialize(L"data/cube.txt", texture_filename_arr))) {
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
        shader_loader_->GetVertexShaderBlobByFileName(L"fontVS.hlsl").Get()));
    text_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"fontPS.hlsl").Get()));

    WCHAR *font_texture[1] = {L"data/font.dds"};
    if (CHECK(text_->LoadFont(L"data/fontdata.txt", font_texture))) {
      MessageBox(hwnd, L"Could not initialize Font data.", L"Error", MB_OK);
      return false;
    }

    DirectX::XMMATRIX base_matrix = {};
    camera_->GetViewMatrix(base_matrix);
    if (CHECK(text_->Initialize(screenWidth, screenHeight, base_matrix))) {
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
        shader_loader_->GetVertexShaderBlobByFileName(L"pbrVS.hlsl").Get()));
    pbr_material->SetPSByteCode(CD3DX12_SHADER_BYTECODE(
        shader_loader_->GetPixelShaderBlobByFileName(L"pbrPS.hlsl").Get()));

    WCHAR *pbr_textures[3] = {L"data/pbr/pbr_albedo.tga",
                              L"data/pbr/pbr_normal.tga",
                              L"data/pbr/pbr_roughmetal.tga"};
    if (CHECK(pbr_model_->Initialize(L"data/pbr/sphere.txt", pbr_textures))) {
      MessageBox(hwnd, L"Could not initialize PBR Model.", L"Error", MB_OK);
      return false;
    }
  }

  return true;
}

void Graphics::Shutdown() {

  if (d3d12_device_) {
    d3d12_device_->WaitForGpuIdle();
  }

  bitmap_.reset();
  text_.reset();
  model_.reset();
  pbr_model_.reset();

  shader_loader_.reset();
  light_.reset();
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
  if (CHECK(text_->SetCpu(cpu_usage_tracker_->GetCpuPercentage()))) {
    return false;
  }

  fps_->Frame();
  if (CHECK(text_->SetFps(fps_->GetFps()))) {
    return false;
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

  float movement_length_sq = XMVectorGetX(XMVector3LengthSq(movement));
  if (movement_length_sq <= 0.0f) {
    return;
  }

  movement = XMVector3Normalize(movement);

  float move_distance = camera_move_speed_ * delta_seconds;
  XMVECTOR position = XMLoadFloat3(&camera_position);
  position = XMVectorAdd(position, XMVectorScale(movement, move_distance));

  XMStoreFloat3(&camera_position, position);
  camera_->SetPosition(camera_position.x, camera_position.y, camera_position.z);
}

bool Graphics::Render() {

  static float rotation = 0.0f;
  rotation += (DirectX::XM_PI * 0.01f);
  if (rotation > DirectX::XM_2PI) {
    rotation -= DirectX::XM_2PI;
  }

  DirectX::XMMATRIX rotate_world = {};
  d3d12_device_->GetWorldMatrix(rotate_world);
  rotate_world = DirectX::XMMatrixRotationY(rotation);
  rotate_world = DirectX::XMMatrixTranspose(rotate_world);

  DirectX::XMMATRIX font_world = {};
  d3d12_device_->GetWorldMatrix(font_world);
  font_world = DirectX::XMMatrixTranspose(font_world);

  DirectX::XMMATRIX projection = {};
  d3d12_device_->GetProjectionMatrix(projection);
  projection = DirectX::XMMatrixTranspose(projection);

  DirectX::XMMATRIX view = {};
  camera_->GetViewMatrix(view);
  view = DirectX::XMMatrixTranspose(view);

  DirectX::XMMATRIX orthogonality = {};
  d3d12_device_->GetOrthoMatrix(orthogonality);
  orthogonality = DirectX::XMMatrixTranspose(orthogonality);

  DirectX::XMMATRIX pbr_world = DirectX::XMMatrixRotationY(rotation * 0.5f) *
                                DirectX::XMMatrixTranslation(3.0f, 0.0f, 0.0f);
  pbr_world = DirectX::XMMatrixTranspose(pbr_world);

  if (CHECK(model_->GetMaterial()->UpdateMatrixConstant(rotate_world, view,
                                                        projection))) {
    return false;
  }

  if (CHECK(model_->GetMaterial()->UpdateLightConstant(
          light_->GetAmbientColor(), light_->GetDiffuseColor(),
          light_->GetDirection()))) {
    return false;
  }

  if (CHECK(model_->GetMaterial()->UpdateFogConstant(3.0f, 6.0f))) {
    return false;
  }

  if (CHECK(text_->GetMaterial()->UpdateMatrixConstant(font_world, view,
                                                       orthogonality))) {
    return false;
  }

  DirectX::XMFLOAT4 pixel_color(1.0f, 0.0f, 0.0f, 0.0f);
  if (CHECK(text_->GetMaterial()->UpdateLightConstant(pixel_color))) {
    return false;
  }

  if (pbr_model_) {
    auto camera_position = camera_->GetPosition();
    auto pbr_material = pbr_model_->GetMaterial();
    if (CHECK(
            pbr_material->UpdateMatrixConstant(pbr_world, view, projection))) {
      return false;
    }
    if (CHECK(pbr_material->UpdateCameraConstant(camera_position))) {
      return false;
    }
    if (CHECK(pbr_material->UpdateLightConstant(pbr_light_direction_))) {
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

  if (CHECK(d3d12_device_->ResetCommandAllocator())) {
    return false;
  }
  if (CHECK(d3d12_device_->ResetCommandList())) {
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

  {
    d3d12_device_->BeginPopulateGraphicsCommandList();

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

  if (CHECK(d3d12_device_->ExecuteDefaultGraphicsCommandList())) {
    return false;
  }

  return true;
}
