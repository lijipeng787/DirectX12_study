#ifndef _FPSCLASS_H_
#define _FPSCLASS_H_

class Fps{
public:
	Fps();

	Fps(const Fps& rhs) = delete;
	
	Fps& operator=(const Fps& rhs) = delete;

	~Fps() {}
public:
	void Frame();
	
	int GetFps() { return fps_value_; }
private:
	int fps_value_ = 0, count_ = 0;

	unsigned long start_time_ = 0;
};

#endif