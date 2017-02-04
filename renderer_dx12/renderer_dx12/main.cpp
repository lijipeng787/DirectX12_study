#include "stdafx.h"
#include "systemclass.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow) {

	auto system = new System();
	if (!system) {
		return 0;
	}
	auto result = system->Initialize();
	if (result) {
		system->Run();
	}

	system->Shutdown();
	delete system;
	system = 0;

	return 0;
}