// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Application bootstrap -- platform entry point.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "app/app.h"

using namespace RetrodevGui;

//
// Entry point (Common)
//
int retromain(int, char**) {
	if (Application::Initialize() == false) {
		return -1;
	}
	Application::Run();
	Application::Shutdown();
	return 0;
}
