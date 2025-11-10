#pragma once

#ifndef _SCENELIGHT_H_
#define _SCENELIGHT_H_

#include <DirectXMath.h>
#include <string>

namespace Lighting {

// Light type enumeration
enum class LightType {
  Directional = 0, // Directional light (sun-like)
  Point = 1,       // Point light (omnidirectional)
  Spot = 2         // Spot light (cone-shaped)
};

// Unified scene light class
// Supports directional, point, and spot lights with complete parameters
class SceneLight {
public:
  SceneLight() = default;

  SceneLight(const SceneLight &rhs) = delete;

  auto operator=(const SceneLight &rhs) -> SceneLight & = delete;

  ~SceneLight() = default;

  // Basic properties
  void SetName(const std::string &name) { name_ = name; }
  auto GetName() const -> const std::string & { return name_; }

  void SetType(LightType type) { type_ = type; }
  auto GetType() const -> LightType { return type_; }

  void SetEnabled(bool enabled) { enabled_ = enabled; }
  auto IsEnabled() const -> bool { return enabled_; }

  // Spatial properties
  void SetPosition(float x, float y, float z) {
    position_ = DirectX::XMFLOAT3(x, y, z);
  }
  void SetPosition(const DirectX::XMFLOAT3 &position) { position_ = position; }
  auto GetPosition() const -> const DirectX::XMFLOAT3 & { return position_; }

  void SetDirection(float x, float y, float z) {
    direction_ = DirectX::XMFLOAT3(x, y, z);
    NormalizeDirection();
  }
  void SetDirection(const DirectX::XMFLOAT3 &direction) {
    direction_ = direction;
    NormalizeDirection();
  }
  auto GetDirection() const -> const DirectX::XMFLOAT3 & { return direction_; }

  // Color and intensity
  void SetColor(float r, float g, float b) {
    color_ = DirectX::XMFLOAT3(r, g, b);
    UpdateColorChannels();
  }
  void SetColor(const DirectX::XMFLOAT3 &color) {
    color_ = color;
    UpdateColorChannels();
  }
  auto GetColor() const -> const DirectX::XMFLOAT3 & { return color_; }

  void SetIntensity(float intensity) {
    intensity_ = intensity;
    UpdateColorChannels();
  }
  auto GetIntensity() const -> float { return intensity_; }

  // Legacy Blinn-Phong interface (for compatibility)
  void SetAmbientColor(float r, float g, float b, float a) {
    ambient_color_ = DirectX::XMFLOAT4(r, g, b, a);
  }
  auto GetAmbientColor() const -> const DirectX::XMFLOAT4 & {
    return ambient_color_;
  }

  void SetDiffuseColor(float r, float g, float b, float a) {
    diffuse_color_ = DirectX::XMFLOAT4(r, g, b, a);
  }
  auto GetDiffuseColor() const -> const DirectX::XMFLOAT4 & {
    return diffuse_color_;
  }

  void SetSpecularColor(float r, float g, float b, float a) {
    specular_color_ = DirectX::XMFLOAT4(r, g, b, a);
  }
  auto GetSpecularColor() const -> const DirectX::XMFLOAT4 & {
    return specular_color_;
  }

  void SetSpecularPower(float power) { specular_power_ = power; }
  auto GetSpecularPower() const -> float { return specular_power_; }

  // Attenuation (for point and spot lights)
  void SetRange(float range) { range_ = range; }
  auto GetRange() const -> float { return range_; }

  void SetAttenuation(float constant, float linear, float quadratic) {
    attenuation_constant_ = constant;
    attenuation_linear_ = linear;
    attenuation_quadratic_ = quadratic;
  }
  auto GetAttenuationConstant() const -> float { return attenuation_constant_; }
  auto GetAttenuationLinear() const -> float { return attenuation_linear_; }
  auto GetAttenuationQuadratic() const -> float {
    return attenuation_quadratic_;
  }

  // Spot light parameters (in degrees)
  void SetSpotAngles(float inner_degrees, float outer_degrees) {
    spot_inner_angle_ = inner_degrees;
    spot_outer_angle_ = outer_degrees;
  }
  auto GetSpotInnerAngle() const -> float { return spot_inner_angle_; }
  auto GetSpotOuterAngle() const -> float { return spot_outer_angle_; }

  // Convenience method: Get combined color * intensity for PBR
  auto GetEffectiveColor() const -> DirectX::XMFLOAT3 {
    return DirectX::XMFLOAT3(color_.x * intensity_, color_.y * intensity_,
                             color_.z * intensity_);
  }

  // Convenience method: Get color as XMFLOAT4 with intensity
  auto GetColorAsFloat4() const -> DirectX::XMFLOAT4 {
    return DirectX::XMFLOAT4(color_.x * intensity_, color_.y * intensity_,
                             color_.z * intensity_, 1.0f);
  }

private:
  void NormalizeDirection() {
    using namespace DirectX;
    XMVECTOR dir = XMLoadFloat3(&direction_);
    dir = XMVector3Normalize(dir);
    XMStoreFloat3(&direction_, dir);
  }

  void UpdateColorChannels() {
    // Update diffuse and specular based on color and intensity
    diffuse_color_ =
        DirectX::XMFLOAT4(color_.x * intensity_, color_.y * intensity_,
                          color_.z * intensity_, 1.0f);

    specular_color_ =
        DirectX::XMFLOAT4(color_.x * intensity_, color_.y * intensity_,
                          color_.z * intensity_, 1.0f);
  }

  // Basic properties
  std::string name_ = "Unnamed Light";

  LightType type_ = LightType::Directional;

  bool enabled_ = true;

  // Spatial properties
  DirectX::XMFLOAT3 position_ = {
      0.0f, 0.0f, 0.0f}; // Position in world space (for point/spot)

  DirectX::XMFLOAT3 direction_ = {0.0f, 0.0f,
                                  1.0f}; // Direction vector (normalized)

  // Color and intensity
  DirectX::XMFLOAT3 color_ = {1.0f, 1.0f, 1.0f}; // RGB color (0-1 range)

  float intensity_ = 1.0f; // Light intensity multiplier

  // Legacy Blinn-Phong parameters (for compatibility with existing shaders)
  DirectX::XMFLOAT4 ambient_color_ = {0.15f, 0.15f, 0.15f, 1.0f};
  DirectX::XMFLOAT4 diffuse_color_ = {1.0f, 1.0f, 1.0f, 1.0f};
  DirectX::XMFLOAT4 specular_color_ = {1.0f, 1.0f, 1.0f, 1.0f};
  float specular_power_ = 32.0f;

  // Attenuation parameters (for point and spot lights)
  float range_ = 100.0f; // Maximum range

  float attenuation_constant_ = 1.0f; // Constant attenuation factor

  float attenuation_linear_ = 0.09f; // Linear attenuation factor

  float attenuation_quadratic_ = 0.032f; // Quadratic attenuation factor

  // Spot light parameters
  float spot_inner_angle_ = 12.5f; // Inner cone angle (degrees)

  float spot_outer_angle_ = 17.5f; // Outer cone angle (degrees)
};

} // namespace Lighting

#endif // !_SCENELIGHT_H_
