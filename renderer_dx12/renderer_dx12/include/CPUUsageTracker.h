#pragma once

#include <pdh.h>

class CPUUsageTracker {
public:
  CPUUsageTracker() {}

  CPUUsageTracker(const CPUUsageTracker &rhs) = delete;

  CPUUsageTracker &operator=(const CPUUsageTracker &rhs) = delete;

  ~CPUUsageTracker() {}

public:
  void Initialize();

  void Shutdown();

  void Update();

  int GetCpuPercentage();

private:
  bool can_read_cpu_;

  HQUERY query_handle_;

  HCOUNTER counter_handle_;

  unsigned long last_sample_time_;

  long cpu_usage_;
};
