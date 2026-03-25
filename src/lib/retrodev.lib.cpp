// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Library entry point -- initialisation and shutdown.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include <retrodev.lib.h>
#include <assets/image/image.h>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

//
// Initialize and shutdown the RetroDev library
//
bool RetrodevLib::RetroDevInit() {
	//
	// Initialize the SDL library with video and audio subsystems.
	//
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
		return false;
	}
	//
	// Verify SDL_image is available by querying its version.
	// SDL3_image does not require an explicit IMG_Init call;
	// format support (PNG, JPG, etc.) is handled automatically.
	//
	int imgVersion = IMG_Version();
	if (imgVersion == 0) {
		SDL_Quit();
		return false;
	}
	return true;
}

//
// Clean up SDL resources before exiting the application.
//
void RetrodevLib::RetroDevShutdown() {
	SDL_Quit();
}
