#ifndef _CAMERACLASS_H_
#define _CAMERACLASS_H_

#include <DirectXMath.h>

class Camera {
public:
  Camera() { Construct2DViewMatrix(); }

  Camera(const Camera &rhs) = delete;

  Camera &operator=(const Camera &rhs) = delete;

  ~Camera() {}

public:
  void SetPosition(float x, float y, float z) {
    position_x_ = x;
    position_y_ = y;
    position_z_ = z;
  }

  void SetRotation(float x, float y, float z) {
    rotation_x_ = x;
    rotation_y_ = y;
    rotation_z_ = z;
  }

  const DirectX::XMFLOAT3 GetPosition() const {
    return DirectX::XMFLOAT3(position_x_, position_y_, position_z_);
  }

  const DirectX::XMFLOAT3 GetRotation() const {
    return DirectX::XMFLOAT3(rotation_x_, rotation_y_, rotation_z_);
  }

  void Update();

  void Construct2DViewMatrix();

  void GetViewMatrix(DirectX::XMMATRIX &view) const { view = view_matrix_; }

  void Get2DViewMatrix(DirectX::XMMATRIX &view) const {
    view = _2D_view_matrix_;
  }

private:
  float position_x_ = 0.0f, position_y_ = 0.0f, position_z_ = 0.0f;

  float rotation_x_ = 0.0f, rotation_y_ = 0.0f, rotation_z_ = 0.0f;

  DirectX::XMMATRIX view_matrix_ = {};

  DirectX::XMMATRIX _2D_view_matrix_ = {};
};

#endif