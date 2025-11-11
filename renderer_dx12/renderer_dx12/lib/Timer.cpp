#include "stdafx.h"

#include "Timer.h"

bool TimerClass::Initialize() {
  // Check to see if this system supports high performance timers.
  QueryPerformanceFrequency((LARGE_INTEGER *)&frequency_);
  if (frequency_ == 0) {
    return false;
  }

  // Find out how many times the frequency counter ticks every millisecond.
  ticks_per_ms_ = static_cast<float>(frequency_ / 1000);

  QueryPerformanceCounter((LARGE_INTEGER *)&start_time_);

  return true;
}

void TimerClass::Update() {
  INT64 currentTime;
  float timeDifference;

  QueryPerformanceCounter((LARGE_INTEGER *)&currentTime);

  timeDifference = static_cast<float>(currentTime - start_time_);

  frame_time_ = timeDifference / ticks_per_ms_;

  start_time_ = currentTime;
}
