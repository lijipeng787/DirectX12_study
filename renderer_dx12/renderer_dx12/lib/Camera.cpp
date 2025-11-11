#include "stdafx.h"

#include "Camera.h"

void Camera::Update() {

  using namespace DirectX;

  XMVECTOR up, position, lookAt;
  float yaw, pitch, roll;
  XMMATRIX rotationMatrix;

  // Setup the vector that points upwards.
  up.m128_f32[0] = 0.0f;
  up.m128_f32[1] = 1.0f;
  up.m128_f32[2] = 0.0f;
  up.m128_f32[3] = 1.0f;

  // Setup the position of the camera in the world.
  position.m128_f32[0] = position_x_;
  position.m128_f32[1] = position_y_;
  position.m128_f32[2] = position_z_;
  position.m128_f32[3] = 1.0f;

  // Setup where the camera is looking by default.
  lookAt.m128_f32[0] = 0.0f;
  lookAt.m128_f32[1] = 0.0f;
  lookAt.m128_f32[2] = 1.0f;
  lookAt.m128_f32[3] = 1.0f;

  // Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in
  // radians.
  pitch = rotation_x_ * 0.0174532925f;
  yaw = rotation_y_ * 0.0174532925f;
  roll = rotation_z_ * 0.0174532925f;

  // Create the rotation matrix from the yaw, pitch, and roll values.
  rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

  // Transform the lookAt and up vector by the rotation matrix so the view is
  // correctly rotated at the origin.
  lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
  up = XMVector3TransformCoord(up, rotationMatrix);

  // Translate the rotated camera position to the location of the viewer.
  lookAt = position + lookAt;

  // Finally create the view matrix from the three updated vectors.
  view_matrix_ = XMMatrixLookAtLH(position, lookAt, up);
}

void Camera::Construct2DViewMatrix() {

  using namespace DirectX;

  XMVECTOR up, position, lookAt;
  float yaw, pitch, roll;
  XMMATRIX rotationMatrix;

  // Setup the vector that points upwards.
  up.m128_f32[0] = 0.0f;
  up.m128_f32[1] = 1.0f;
  up.m128_f32[2] = 0.0f;
  up.m128_f32[3] = 1.0f;

  // Setup the position of the camera in the world.
  position.m128_f32[0] = position_x_;
  position.m128_f32[1] = position_y_;
  position.m128_f32[2] = position_z_;
  position.m128_f32[3] = 1.0f;

  // Setup where the camera is looking by default.
  lookAt.m128_f32[0] = 0.0f;
  lookAt.m128_f32[1] = 0.0f;
  lookAt.m128_f32[2] = 1.0f;
  lookAt.m128_f32[3] = 1.0f;

  // Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in
  // radians.
  pitch = rotation_x_ * 0.0174532925f;
  yaw = rotation_y_ * 0.0174532925f;
  roll = rotation_z_ * 0.0174532925f;

  // Create the rotation matrix from the yaw, pitch, and roll values.
  rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

  // Transform the lookAt and up vector by the rotation matrix so the view is
  // correctly rotated at the origin.
  lookAt = XMVector3TransformCoord(lookAt, rotationMatrix);
  up = XMVector3TransformCoord(up, rotationMatrix);

  // Translate the rotated camera position to the location of the viewer.
  lookAt = position + lookAt;

  // Finally create the view matrix from the three updated vectors.
  view_matrix_2d_ = XMMatrixLookAtLH(position, lookAt, up);
}

void Camera::UpdateReflection(float height) {
  using namespace DirectX;

  XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

  float reflected_y = -position_y_ + (height * 2.0f);
  XMVECTOR position =
      XMVectorSet(position_x_, reflected_y, position_z_, 1.0f);

  float yaw = rotation_y_ * 0.0174532925f;
  XMVECTOR look_at = XMVectorSet(sinf(yaw) + position_x_, reflected_y,
                                 cosf(yaw) + position_z_, 1.0f);

  reflection_view_matrix_ = XMMatrixLookAtLH(position, look_at, up);
}