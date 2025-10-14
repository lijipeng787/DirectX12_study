#pragma once

#ifndef APPLICATION_HEADER_POSITION_H

#define APPLICATION_HEADER_POSITION_H

class Position {
public:
  Position() {}

  Position(const Position &rhs) = delete;

  Position &operator=(const Position &rhs) = delete;

  ~Position() {}

public:
  void Position::SetPosition(float x, float y, float z) {
    position_x_ = x;
    position_y_ = y;
    position_z_ = z;
  }

  void Position::SetRotation(float x, float y, float z) {
    rotation_x_ = x;
    rotation_y_ = y;
    rotation_z_ = z;
  }

  void Position::GetPosition(float &x, float &y, float &z) {
    x = position_x_;
    y = position_y_;
    z = position_z_;
  }

  void Position::GetRotation(float &x, float &y, float &z) {
    x = rotation_x_;
    y = rotation_y_;
    z = rotation_z_;
  }

  void Position::SetFrameTime(float frame_time) { frame_time_ = frame_time; }

  void MoveForward(bool key_down);

  void MoveBackward(bool key_down);

  void MoveUpward(bool key_down);

  void MoveDownward(bool key_down);

  void TurnLeft(bool key_down);

  void TurnRight(bool key_down);

  void LookUpward(bool key_down);

  void LookDownward(bool key_down);

private:
  float position_x_ = {}, position_y_ = {}, position_z_ = {};

  float rotation_x_ = {}, rotation_y_ = {}, rotation_z_ = {};

  float frame_time_ = {};

  float forward_speed_ = {}, backward_speed_ = {};

  float upward_speed_ = {}, downward_speed_ = {};

  float turn_left_speed_ = {}, turn_right_speed_ = {};

  float look_up_speed_ = {}, look_down_speed_ = {};
};

#endif // !APPLICATION_HEADER_POSITION_H
