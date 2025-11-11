#pragma once

#include <DirectXMath.h>

class Light {
public:
  Light() {}

  Light(const Light &rhs) = delete;

  Light operator=(const Light &rhs) = delete;

  ~Light() {}

public:
  void Light::SetAmbientColor(float red, float green, float blue, float alpha) {
    ambient_color_ = DirectX::XMFLOAT4(red, green, blue, alpha);
  }

  void Light::SetDiffuseColor(float red, float green, float blue, float alpha) {
    diffuse_color_ = DirectX::XMFLOAT4(red, green, blue, alpha);
  }

  void Light::SetDirection(float x, float y, float z) {
    direction_ = DirectX::XMFLOAT3(x, y, z);
  }

  void SetSpecularColor(float red, float green, float blue, float alpha) {
    specular_color_ = DirectX::XMFLOAT4(red, green, blue, alpha);
  }

  void SetSpecularPower(float power) { specular_power_ = power; }

  DirectX::XMFLOAT4 Light::GetAmbientColor() const { return ambient_color_; }

  DirectX::XMFLOAT4 Light::GetDiffuseColor() const { return diffuse_color_; }

  DirectX::XMFLOAT3 Light::GetDirection() const { return direction_; }

  DirectX::XMFLOAT4 GetSpecularColor() const { return specular_color_; }

  float GetSpecularPower() const { return specular_power_; }

private:
  DirectX::XMFLOAT4 ambient_color_ = {};

  DirectX::XMFLOAT4 diffuse_color_ = {};

  DirectX::XMFLOAT3 direction_ = {};

  DirectX::XMFLOAT4 specular_color_ = {};

  float specular_power_ = 0.0f;
};
