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
  bool m_canReadCpu;

  HQUERY m_queryHandle;

  HCOUNTER m_counterHandle;

  unsigned long m_lastSampleTime;

  long m_cpuUsage;
};
