#pragma once

#include <windows.h>

class TimerClass {
public:
  TimerClass() {}

  TimerClass(const TimerClass &rhs) = delete;

  TimerClass &operator=(const TimerClass &rhs) = delete;

  ~TimerClass() {}

public:
  bool Initialize();

  void Update();

  const float GetTime() const { return frame_time_; }

private:
  INT64 frequency_ = 0;

  float ticks_per_ms_ = 0.0f;

  INT64 start_time_ = 0;

  float frame_time_ = 0.0f;
};
