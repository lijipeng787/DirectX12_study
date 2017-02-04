#ifndef HEADER_GRAPHICSCLASS_H
#define HEADER_GRAPHICSCLASS_H

#include <memory>
#include "../d3d12 common/DirectX12Device.h"
#include "../d3d12 common/Camera.h"
#include "../d3d12 common/Input.h"
#include "../d3d12 common/Fps.h"
#include "../d3d12 common/Light.h"
#include "../d3d12 common/Cpu.h"

#include "modelclass.h"
#include "AmbientLightingShaderLoader.h"
#include "fontshaderclass.h"
#include "textclass.h"
#include "BitMap.h"
#include "BitMapShaderLoader.h"

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;

class Graphics{

public:
	Graphics() {}

	Graphics(const Graphics& rhs) = delete;
	
	Graphics operator=(const Graphics& rhs) = delete;

	~Graphics() {}

public:
	bool Initialize(int, int, HWND);
	
	void Shutdown();
	
	bool Frame();

private:
	bool Render();

private:
	DirectX12Device* d3d12_device_ = nullptr;

	std::shared_ptr<Light> light_ = nullptr;
	
	std::shared_ptr<Camera> camera_ = nullptr;

	std::shared_ptr<Input> input_ = nullptr;

	std::shared_ptr<AmbientLightingShaderLoader> ambient_light_shader_ = nullptr;

	std::shared_ptr<FontShader> font_shader_ = nullptr;

	std::shared_ptr<BitMapShaderLoader> bitmap_shader_ = nullptr;

	std::shared_ptr<Bitmap> bitmap_ = nullptr;

	std::shared_ptr<Text> text_ = nullptr;

	std::shared_ptr<Model> model_ = nullptr;

	std::shared_ptr<Fps> fps_ = nullptr;

	std::shared_ptr<Cpu> cpu_ = nullptr;
};

#endif //!HEADER_GRAPHICSCLASS_H