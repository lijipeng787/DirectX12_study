#include "stdafx.h"
#include "Fps.h"

#include <windows.h>
#include <mmsystem.h>

Fps::Fps() {
	start_time_ = timeGetTime();
}

void Fps::Frame() {

	count_++;

	if (timeGetTime() >= (start_time_ + 1000)) {
		fps_value_ = count_;
		count_ = 0;
		start_time_ = timeGetTime();
	}
}
