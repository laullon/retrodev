// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Main view -- menu bar.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>

namespace RetrodevGui {

	class MainViewMenu {
	public:
		static bool Perform();
		//
		// Request application quit. Shows a confirmation dialog if there are unsaved changes.
		// Safe to call from the SDL event loop or from UI code.
		//
		static void RequestQuit();
		//
		// Returns true when the application should exit the run loop.
		//
		static bool ShouldQuit();

		//
		// Must be called during initialization (before the first frame) to register
		// the INI settings handler that persists the last used project folder.
		//
		static void RegisterSettingsHandler();

	private:
		//
		// Set to true when Close Project is selected, triggers the confirmation modal next frame
		//
		static inline bool m_closeProjectPending = false;
		//
		// Set to true once quit is confirmed, causes the run loop to exit
		//
		static inline bool m_quitPending = false;
		//
		// Renders the build toolbar (combo + Build + Debug buttons) inside the menu bar
		//
		static void PerformBuildToolbar();
	};

}