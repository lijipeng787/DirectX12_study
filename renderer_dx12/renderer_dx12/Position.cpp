#include "stdafx.h"

#include "Position.h"

void Position::MoveForward(bool key_down) {
	if (key_down) {
		forward_speed_ = (forward_speed_ < frame_time_*1.0f) ? (forward_speed_ += frame_time_*0.001f) : frame_time_*1.0f;
	}
	else {
		forward_speed_ = 0.0f;
	}

	auto radians = rotation_y_*0.0174532925f;

	position_x_ += sinf(radians)*forward_speed_;

	position_z_ += cosf(radians)*forward_speed_;
}

void Position::MoveBackward(bool key_down) {
	if (key_down) {
		backward_speed_ = (backward_speed_ < frame_time_*1.0f) ? (backward_speed_ += frame_time_*0.001f) : frame_time_*1.0f;
	}
	else {
		backward_speed_ = 0.0f;
	}

	auto radians = rotation_y_*0.0174532925f;

	position_x_ -= sinf(radians)*backward_speed_;

	position_z_ -= cosf(radians)*backward_speed_;
}

void Position::MoveUpward(bool key_down){
	if (key_down) {
		upward_speed_ = (upward_speed_ < frame_time_*1.0f) ? (upward_speed_ += frame_time_*0.001f) : frame_time_*1.0f;
	}
	else {
		upward_speed_ = 0.0f;
	}

	position_y_ += upward_speed_;
}

void Position::MoveDownward(bool key_down){
	if (key_down) {
		downward_speed_ = (downward_speed_ < frame_time_*1.0f) ? (downward_speed_ += frame_time_*0.001f) : frame_time_*1.0f;
	}
	else {
		downward_speed_ = 0.0f;
	}

	position_y_ -= downward_speed_;
}

void Position::TurnLeft(bool key_down){
	if (key_down) {
		turn_left_speed_ = (turn_left_speed_ < frame_time_*1.0f) ? turn_left_speed_ += frame_time_*0.001f : frame_time_*1.0f;
	}
	else {
		turn_left_speed_ = 0.0f;
	}

	rotation_y_ -= turn_left_speed_;
	if (rotation_y_ < 0.0f)
		rotation_y_ += 360.0f;
}

void Position::TurnRight(bool key_down){
	if (key_down) {
		turn_right_speed_ = (turn_right_speed_ < frame_time_*1.0f) ? turn_right_speed_ += frame_time_*0.001f : frame_time_*1.0f;
	}
	else {
		turn_right_speed_ = 0.0f;
	}

	rotation_y_ += turn_right_speed_;
	if (rotation_y_ > 360.0f)
		rotation_y_ -= 360.0f;
}

void Position::LookUpward(bool key_down) {
	if (key_down) {
		look_up_speed_ = (look_up_speed_ < frame_time_*75.0f) ? look_up_speed_ += frame_time_*0.001f : frame_time_*75.0f;
	}
	else {
		look_up_speed_ = 0.0f;
	}

	rotation_x_ -= look_up_speed_;
	if (rotation_x_ > 90.0f)
		rotation_x_ = 90.0f;
}

void Position::LookDownward(bool key_down){
	if (key_down) {
		look_down_speed_ = (look_down_speed_ < frame_time_*75.0f) ? look_down_speed_ += frame_time_*0.001f : frame_time_*75.0f;
	}
	else {
		look_down_speed_ = 0.0f;
	}

	rotation_x_ += look_down_speed_;
	if (rotation_x_ < -90.0f)
		rotation_x_ = -90.0f;
}
