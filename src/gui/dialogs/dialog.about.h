// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// About dialog -- version and credits popup.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <SDL3/SDL.h>

namespace RetrodevGui {

	class AboutDialog {
	public:
		//
		// Open the dialog on the next Render() call
		//
		static void Show();
		//
		// Render the dialog; call every frame from the menu render function
		//
		static void Render();

	private:
		//
		// Render a clickable hyperlink that opens a URL in the default browser
		//
		static void RenderLink(const char* label, const char* url);
		//
		// True when the dialog should open on the next frame
		//
		static inline bool m_open = false;
		//
		// Optional logo texture loaded from the "gui.res.images.about" embedded resource
		//
		static inline SDL_Texture* m_logoTexture = nullptr;
		//
		// True once the resource load has been attempted (success or failure)
		//
		static inline bool m_logoLoaded = false;
	};

}
