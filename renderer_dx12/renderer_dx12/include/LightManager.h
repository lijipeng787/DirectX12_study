#pragma once

#include "SceneLight.h"
#include <memory>
#include <string>
#include <vector>

namespace Lighting {

// Light manager for managing multiple scene lights
// Supports adding, removing, and querying lights
class LightManager {
public:
  LightManager() = default;

  LightManager(const LightManager &rhs) = delete;

  auto operator=(const LightManager &rhs) -> LightManager & = delete;

  ~LightManager() = default;

  // Maximum number of lights (for shader compatibility)
  void SetMaxLights(size_t max_lights) { max_lights_ = max_lights; }

  auto GetMaxLights() const -> size_t { return max_lights_; }

  // Add a new light and return its index
  // Returns -1 if max lights reached
  auto AddLight(std::shared_ptr<SceneLight> light) -> int {
    if (!light || lights_.size() >= max_lights_) {
      return -1;
    }
    lights_.push_back(light);
    return static_cast<int>(lights_.size() - 1);
  }

  // Create and add a new light with default parameters
  auto CreateLight(const std::string &name = "Light",
                   LightType type = LightType::Directional)
      -> std::shared_ptr<SceneLight> {
    auto light = std::make_shared<SceneLight>();
    light->SetName(name);
    light->SetType(type);

    if (AddLight(light) >= 0) {
      return light;
    }
    return nullptr;
  }

  // Remove light by index
  auto RemoveLight(size_t index) -> bool {
    if (index >= lights_.size()) {
      return false;
    }
    lights_.erase(lights_.begin() + index);
    return true;
  }

  // Remove light by pointer
  auto RemoveLight(const std::shared_ptr<SceneLight> &light) -> bool {
    auto it = std::find(lights_.begin(), lights_.end(), light);
    if (it != lights_.end()) {
      lights_.erase(it);
      return true;
    }
    return false;
  }

  // Clear all lights
  void Clear() { lights_.clear(); }

  // Get light by index
  std::shared_ptr<SceneLight> GetLight(size_t index) const {
    if (index >= lights_.size()) {
      return nullptr;
    }
    return lights_[index];
  }

  // Get the first enabled light of a specific type
  auto GetFirstLightOfType(LightType type) const
      -> std::shared_ptr<SceneLight> {
    for (const auto &light : lights_) {
      if (light && light->IsEnabled() && light->GetType() == type) {
        return light;
      }
    }
    return nullptr;
  }

  // Get all lights
  const std::vector<std::shared_ptr<SceneLight>> &GetAllLights() const {
    return lights_;
  }

  // Get number of lights
  auto GetLightCount() const -> size_t { return lights_.size(); }

  // Get number of enabled lights
  auto GetEnabledLightCount() const -> size_t {
    size_t count = 0;
    for (const auto &light : lights_) {
      if (light && light->IsEnabled()) {
        ++count;
      }
    }
    return count;
  }

  // Get the primary/main light (typically the first directional light)
  auto GetPrimaryLight() const -> std::shared_ptr<SceneLight> {
    // Try to find first enabled directional light
    auto directional = GetFirstLightOfType(LightType::Directional);
    if (directional) {
      return directional;
    }

    // Otherwise return first enabled light of any type
    for (const auto &light : lights_) {
      if (light && light->IsEnabled()) {
        return light;
      }
    }

    return nullptr;
  }

  // Enable/disable all lights
  void SetAllLightsEnabled(bool enabled) {
    for (auto &light : lights_) {
      if (light) {
        light->SetEnabled(enabled);
      }
    }
  }

  // Get lights by type
  auto GetLightsByType(LightType type) const
      -> std::vector<std::shared_ptr<SceneLight>> {
    std::vector<std::shared_ptr<SceneLight>> result;
    for (const auto &light : lights_) {
      if (light && light->GetType() == type) {
        result.push_back(light);
      }
    }
    return result;
  }

  // Get enabled lights by type
  auto GetEnabledLightsByType(LightType type) const
      -> std::vector<std::shared_ptr<SceneLight>> {
    std::vector<std::shared_ptr<SceneLight>> result;
    for (const auto &light : lights_) {
      if (light && light->IsEnabled() && light->GetType() == type) {
        result.push_back(light);
      }
    }
    return result;
  }

private:
  std::vector<std::shared_ptr<SceneLight>> lights_;

  size_t max_lights_{8};
};

} // namespace Lighting

