#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <memory>
#include <type_traits>

// Forward declarations
class DirectX12Device;
class Camera;

namespace Lighting {
class LightManager;
class SceneLight;
} // namespace Lighting

namespace ResourceLoader {
class ShaderLoader;
} // namespace ResourceLoader

// Scene context structures for unified interface
struct SceneInitializeContext {
  std::shared_ptr<DirectX12Device> device = nullptr;
  std::shared_ptr<ResourceLoader::ShaderLoader> shader_loader = nullptr;
  std::shared_ptr<Lighting::LightManager> light_manager = nullptr;
  std::shared_ptr<Camera> camera = nullptr;
  HWND hwnd = nullptr;
};

struct SceneUpdateContext {
  float delta_seconds = 0.0f;
};

struct SceneRenderContext {
  const DirectX::XMMATRIX &view;
  const DirectX::XMMATRIX &projection;
  Lighting::SceneLight *primary_light = nullptr;
};

struct SceneReflectionContext {
  const DirectX::XMMATRIX &projection;
};

// Type-erased Scene wrapper using external polymorphism
// This allows heterogeneous scene storage without virtual inheritance
class Scene {
public:
  // Constructor from any concrete scene type
  template <typename T>
  explicit Scene(std::shared_ptr<T> concrete_scene)
      : impl_(std::make_unique<Model<T>>(std::move(concrete_scene))) {}

  // Copy/Move semantics
  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;

  Scene(Scene &&) noexcept = default;
  Scene &operator=(Scene &&) noexcept = default;

  ~Scene() = default;

  // Unified scene interface
  bool Initialize(const SceneInitializeContext &ctx) {
    return impl_->Initialize(ctx);
  }

  void Shutdown() { impl_->Shutdown(); }

  void Update(const SceneUpdateContext &ctx) { impl_->Update(ctx); }

  bool Render(const SceneRenderContext &ctx) { return impl_->Render(ctx); }

  void SetRotationAngle(float radians) { impl_->SetRotationAngle(radians); }

  // Optional reflection pass (returns true if not supported)
  bool RenderReflection(const SceneReflectionContext &ctx) {
    return impl_->RenderReflection(ctx);
  }

  // Optional resize handler
  void OnResize(int width, int height) {
    impl_->OnResize(width, height);
  }

private:
  // Internal concept interface (virtual base)
  struct Concept {
    virtual ~Concept() = default;

    virtual bool Initialize(const SceneInitializeContext &ctx) = 0;
    virtual void Shutdown() = 0;
    virtual void Update(const SceneUpdateContext &ctx) = 0;
    virtual bool Render(const SceneRenderContext &ctx) = 0;
    virtual void SetRotationAngle(float radians) = 0;
    virtual bool RenderReflection(const SceneReflectionContext &ctx) = 0;
    virtual void OnResize(int width, int height) = 0;
  };

  // Internal model implementation (wraps concrete scene)
  template <typename T> struct Model : Concept {
    std::shared_ptr<T> scene_;

    explicit Model(std::shared_ptr<T> scene) : scene_(std::move(scene)) {}

    bool Initialize(const SceneInitializeContext &ctx) override {
      return scene_->Initialize(ctx);
    }

    void Shutdown() override { scene_->Shutdown(); }

    void Update(const SceneUpdateContext &ctx) override {
      scene_->Update(ctx.delta_seconds);
    }

    bool Render(const SceneRenderContext &ctx) override {
      return scene_->Render(ctx);
    }

    void SetRotationAngle(float radians) override {
      scene_->SetRotationAngle(radians);
    }

    bool RenderReflection(const SceneReflectionContext &ctx) override {
      // SFINAE: check if T has RenderReflection method
      if constexpr (HasRenderReflection<T>) {
        return scene_->RenderReflection(ctx);
      } else {
        // Default: no reflection pass needed
        return true;
      }
    }

    void OnResize(int width, int height) override {
      if constexpr (HasOnResize<T>) {
        scene_->OnResize(width, height);
      }
    }

  private:
    // Type trait to detect RenderReflection method
    template <typename U, typename = void>
    struct HasRenderReflectionImpl : std::false_type {};

    template <typename U>
    struct HasRenderReflectionImpl<
        U, std::void_t<decltype(std::declval<U>().RenderReflection(
               std::declval<const SceneReflectionContext &>()))>>
        : std::true_type {};

    template <typename U>
    static constexpr bool HasRenderReflection =
        HasRenderReflectionImpl<U>::value;

    // Type trait to detect OnResize method
    template <typename U, typename = void>
    struct HasOnResizeImpl : std::false_type {};

    template <typename U>
    struct HasOnResizeImpl<
        U, std::void_t<decltype(std::declval<U>().OnResize(0, 0))>>
        : std::true_type {};

    template <typename U>
    static constexpr bool HasOnResize =
        HasOnResizeImpl<U>::value;
  };

  // Type-erased pointer to concrete scene
  std::unique_ptr<Concept> impl_;
};

