#ifndef HEADER_GRAPHICSCLASS_H
#define HEADER_GRAPHICSCLASS_H

#include <Windows.h>
#include <memory>

#include "ShaderLoader.h"

constexpr bool FULL_SCREEN = false;
constexpr bool VSYNC_ENABLED = true;
constexpr float SCREEN_DEPTH = 1000.0f;
constexpr float SCREEN_NEAR = 0.1f;

class DirectX12Device;
class Light;
class Camera;
class Input;
class ScreenQuad;
class Text;
class Model;
class Fps;
class Cpu;

class Graphics {
public:
  Graphics() {}

  Graphics(const Graphics &rhs) = delete;

  Graphics operator=(const Graphics &rhs) = delete;

  ~Graphics() {}

public:
  bool Initialize(int, int, HWND);

  void Shutdown();

  bool Frame();

private:
  bool Render();

private:
  DirectX12Device *d3d12_device_ = nullptr;

  std::shared_ptr<Light> light_ = nullptr;

  std::shared_ptr<Camera> camera_ = nullptr;

  std::shared_ptr<Input> input_ = nullptr;

  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader_ = nullptr;

  std::shared_ptr<ScreenQuad> bitmap_ = nullptr;

  std::shared_ptr<Text> text_ = nullptr;

  std::shared_ptr<Model> model_ = nullptr;

  std::shared_ptr<Fps> fps_ = nullptr;

  std::shared_ptr<Cpu> cpu_ = nullptr;
};

#endif //! HEADER_GRAPHICSCLASS_H