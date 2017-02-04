#include "stdafx.h"
#include "graphicsclass.h"
#include <new>

bool Graphics::Initialize(int screenWidth, int screenHeight, HWND hwnd){

	d3d12_device_ = DirectX12Device::GetD3d12DeviceInstance();
	if (!d3d12_device_)
	{
		return false;
	}
	
	if (CHECK(d3d12_device_->Initialize(screenWidth, screenHeight, VSYNC_ENABLED, hwnd, FULL_SCREEN, SCREEN_DEPTH, SCREEN_NEAR))) {
		MessageBox(hwnd, L"Could not initialize Direct3D.", L"Error", MB_OK);
		return false;
	}

	cpu_ = std::make_shared<Cpu>();
	if (!cpu_) {
		MessageBox(hwnd, L"Could not initialize cpu status detector.", L"Error", MB_OK);
		return false;
	}

	cpu_->Initialize();

	fps_ = std::make_shared<Fps>();
	if (!fps_) {
		MessageBox(hwnd, L"Could not initialize fps detector.", L"Error", MB_OK);
		return false;
	}

	camera_ = std::make_shared<Camera>();
	if (!camera_) {
		return false;
	}
	camera_->SetPosition(0.0f, 0.0f, -5.0f);

	light_ = std::make_shared<Light>();
	if (!light_) {
		return false;
	}

	light_->SetAmbientColor(0.15f, 0.15f, 0.15f, 1.0f);
	light_->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f);
	light_->SetDirection(0.0f, 0.0f, 1.0f);

	bitmap_shader_ = std::make_shared<BitMapShaderLoader>();
	if (!bitmap_shader_) {
		return false;
	}
	if (CHECK(bitmap_shader_->Initialize(L"../../tut22/texture.vs", L"../../tut22/texture.ps"))) {
		MessageBox(hwnd, L"Could not initialize Light Shader.", L"Error", MB_OK);
		return false;
	}

	bitmap_ = std::make_shared<Bitmap>();
	if (!bitmap_) {
		return false;
	}

	bitmap_->SetVertexShader(CD3DX12_SHADER_BYTECODE(bitmap_shader_->GetVertexShaderBlob().Get()));
	bitmap_->SetPixelShader(CD3DX12_SHADER_BYTECODE(bitmap_shader_->GetPixelSHaderBlob().Get()));
	if (CHECK(bitmap_->Initialize(screenWidth, screenHeight, 255, 255))) {
		MessageBox(hwnd, L"Could not initialize Bitmap.", L"Error", MB_OK);
		return false;
	}

	ambient_light_shader_ = std::make_shared<AmbientLightingShaderLoader>();
	if (!ambient_light_shader_) {
		return false;
	}
	if (CHECK(ambient_light_shader_->Initialize(L"../../tut22/light.vs", L"../../tut22/light.ps"))) {
		MessageBox(hwnd, L"Could not initialize Light Shader.", L"Error", MB_OK);
		return false;
	}

	font_shader_ = std::make_shared<FontShader>();
	if (!font_shader_) {
		return false;
	}
	if (CHECK(font_shader_->Initialize(L"../../tut22/font.vs", L"../../tut22/font.ps"))) {
		MessageBox(hwnd, L"Could not initialize Font Shader.", L"Error", MB_OK);
		return false;
	}

	model_ = std::make_shared<Model>();
	if (!model_) {
		return false;
	}

	model_->SetVertexShader(CD3DX12_SHADER_BYTECODE(ambient_light_shader_->GetVertexShaderBlob().Get()));
	model_->SetPixelShader(CD3DX12_SHADER_BYTECODE(ambient_light_shader_->GetPixelSHaderBlob().Get()));

	WCHAR* texture_filename_arr[3] = { L"../../tut22/data/stone01.dds", L"../../tut22/data/dirt01.dds", L"../../tut22/data/alpha01.dds" };

	if (CHECK(model_->Initialize(L"../../tut22/data/cube.txt", texture_filename_arr))) {
		MessageBox(hwnd, L"Could not initialize Model.", L"Error", MB_OK);
		return false;
	}

	text_ = std::make_shared<Text>();
	if (!text_) {
		MessageBox(hwnd, L"Could not initialize Text object.", L"Error", MB_OK);
		return false;
	}

	WCHAR *font_texture[1] = { L"../../tut22/data/font.dds" };

	if (CHECK(text_->LoadFont(L"../../tut22/data/fontdata.txt", font_texture))) {
		MessageBox(hwnd, L"Could not initialize Font data.", L"Error", MB_OK);
		return false;
	}

	DirectX::XMMATRIX base_matrix = {};
	camera_->GetViewMatrix(base_matrix);
	if (CHECK(text_->Initialize(screenWidth, screenHeight, base_matrix))) {
		return false;
	}

	text_->SetVertexShader(CD3DX12_SHADER_BYTECODE(font_shader_->GetVertexShaderBlob().Get()));
	text_->SetPixelShader(CD3DX12_SHADER_BYTECODE(font_shader_->GetPixelSHaderBlob().Get()));

	text_->InitializeRootSignature();
	text_->InitializeGraphicsPipelineState();
	text_->InitializeConstantBuffer();

	return true;
}


void Graphics::Shutdown(){

	if(d3d12_device_){
		delete d3d12_device_;
		d3d12_device_ = nullptr;
	}
}

bool Graphics::Frame(){

	cpu_->Frame();
	if (CHECK(text_->SetCpu(cpu_->GetCpuPercentage()))) {
		return false;
	}

	fps_->Frame();
	if (CHECK(text_->SetFps(fps_->GetFps()))) {
		return false;
	}

	if (FAILED(Render())) {
		return false;
	}

	return true;
}


bool Graphics::Render() {

	static float rotation = 0.0f;
	rotation += (DirectX::XM_PI * 0.01f);
	if (rotation > 360.0f) {
		rotation -= 360.0f;
	}

	camera_->Update();

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

	if (CHECK(model_->UpdateMatrixConstant(rotate_world, view, projection))) {
		return false;
	}

	if (CHECK(model_->UpdateLightConstant(light_->GetAmbientColor(), light_->GetDiffuseColor(), light_->GetDirection()))) {
		return false;
	}

	if (CHECK(model_->UpdateFogConstant(-0.5f, 10.0f))) {
		return false;
	}

	if (CHECK(text_->UpdateMatrixConstant(font_world, view, orthogonality))) {
		return false;
	}
	DirectX::XMFLOAT4 pixel_color(1.0f, 0.0f, 0.0f, 0.0f);
	if (CHECK(text_->UpdateLightConstant(pixel_color))) {
		return false;
	}

	auto off_screen_root_signature = bitmap_->GetRootSignature();
	auto off_screen_pso = bitmap_->GetPipelineStateObject();

	auto font_root_signature = text_->GetRootSignature();
	auto blend_enabled_pso = text_->GetBlendEnabledPso();
	auto font_matrix_constant = text_->GetMatrixConstantBuffer();
	auto font_pixel_constant = text_->GetPixelConstantBuffer();

	auto light_root_signature = model_->GetRootSignature();
	auto pso = model_->GetPipelineStateObject();
	auto light_matrix_constant = model_->GetMatrixConstantBuffer();
	auto light_constant = model_->GetLightConstantBuffer();
	auto fog_constant = model_->GetFogConstantBuffer();

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

		ID3D12DescriptorHeap *light_shader_heap[] = { model_.get()->GetShaderRescourceView().Get() };
		d3d12_device_->SetDescriptorHeaps(1, light_shader_heap);

		d3d12_device_->SetGraphicsRootDescriptorTable(0, light_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
		d3d12_device_->SetGraphicsRootConstantBufferView(1, light_matrix_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(2, light_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(3, fog_constant->GetGPUVirtualAddress());

		d3d12_device_->BindIndexBuffer(&model_->GetIndexBufferView());
		d3d12_device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());

		d3d12_device_->Draw(model_->GetIndexCount());

		d3d12_device_->SetGraphicsRootSignature(font_root_signature);
		d3d12_device_->SetPipelineStateObject(blend_enabled_pso);

		ID3D12DescriptorHeap *font_shader_heap[] = { text_.get()->GetShaderRescourceView().Get() };
		d3d12_device_->SetDescriptorHeaps(1, font_shader_heap);

		d3d12_device_->SetGraphicsRootDescriptorTable(0, font_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
		d3d12_device_->SetGraphicsRootConstantBufferView(1, font_matrix_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(2, font_pixel_constant->GetGPUVirtualAddress());

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

		d3d12_device_->SetGraphicsRootSignature(light_root_signature);
		d3d12_device_->SetPipelineStateObject(pso);

		ID3D12DescriptorHeap *light_shader_heap[] = { model_.get()->GetShaderRescourceView().Get() };
		d3d12_device_->SetDescriptorHeaps(1, light_shader_heap);

		d3d12_device_->SetGraphicsRootDescriptorTable(0, light_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
		d3d12_device_->SetGraphicsRootConstantBufferView(1, light_matrix_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(2, light_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(3, fog_constant->GetGPUVirtualAddress());

		d3d12_device_->BindIndexBuffer(&model_->GetIndexBufferView());
		d3d12_device_->BindVertexBuffer(0, 1, &model_->GetVertexBufferView());

		d3d12_device_->Draw(model_->GetIndexCount());

		d3d12_device_->SetGraphicsRootSignature(font_root_signature);
		d3d12_device_->SetPipelineStateObject(blend_enabled_pso);

		ID3D12DescriptorHeap *font_shader_heap[] = { text_.get()->GetShaderRescourceView().Get() };
		d3d12_device_->SetDescriptorHeaps(1, font_shader_heap);

		d3d12_device_->SetGraphicsRootDescriptorTable(0, font_shader_heap[0]->GetGPUDescriptorHandleForHeapStart());
		d3d12_device_->SetGraphicsRootConstantBufferView(1, font_matrix_constant->GetGPUVirtualAddress());
		d3d12_device_->SetGraphicsRootConstantBufferView(2, font_pixel_constant->GetGPUVirtualAddress());

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

		bitmap_->UpdateConstantBuffer(font_world, view, orthogonality);
		bitmap_->UpdateBitmapPos(100, 100);

		auto off_screen_heap = d3d12_device_->GetOffScreenTextureHeapView();
		ID3D12DescriptorHeap *off_screen_descriptor_heap[] = { off_screen_heap.Get() };
		d3d12_device_->SetDescriptorHeaps(1, off_screen_descriptor_heap);
		d3d12_device_->SetGraphicsRootDescriptorTable(0, off_screen_descriptor_heap[0]->GetGPUDescriptorHandleForHeapStart());
		d3d12_device_->SetGraphicsRootConstantBufferView(1, bitmap_->GetConstantBuffer()->GetGPUVirtualAddress());

		d3d12_device_->Draw(bitmap_->GetIndexCount());

		d3d12_device_->EndPopulateGraphicsCommandList();
	}

	if (CHECK(d3d12_device_->ExecuteDefaultGraphicsCommandList())) {
		return false;
	}

	return true;
}
