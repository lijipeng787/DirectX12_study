#include "Fps.h"

void Fps::Frame() {

	count_++;

	if (timeGetTime() >= (start_time_ + 1000)) {
		fps_value_ = count_;
		count_ = 0;
		start_time_ = timeGetTime();
	}
}
