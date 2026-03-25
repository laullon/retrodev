// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Command-line entry point.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "app/app.h"

//
// Entry point
//
int main(int, char**) {
	Application* app = new Application();

	if (app->Initialize() == false) {
		return -1;
	}
	app->Run();
	app->Shutdown();
	return 0;
}
