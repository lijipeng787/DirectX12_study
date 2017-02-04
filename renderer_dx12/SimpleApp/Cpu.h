#ifndef _CPUCLASS_H_
#define _CPUCLASS_H_

#include <pdh.h>

class Cpu{
public:
	Cpu() {}
	
	Cpu(const Cpu& rhs) = delete;

	Cpu& operator=(const Cpu& rhs) = delete;
	
	~Cpu() {}
public:
	void Initialize();
	
	void Shutdown();
	
	void Frame();
	
	int GetCpuPercentage();
private:
	bool m_canReadCpu;
	
	HQUERY m_queryHandle;
	
	HCOUNTER m_counterHandle;
	
	unsigned long m_lastSampleTime;
	
	long m_cpuUsage;
};

#endif