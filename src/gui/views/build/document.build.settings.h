// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

namespace RetrodevGui {

	//
	// Machine-local emulator executable paths.
	// Stored in retrodev.ini (via ImGui settings handler) so they are NOT saved
	// into the project file — the same project can be used on different machines
	// where emulators may be installed in different locations.
	//
	class EmulatorSettings {
	public:
		//
		// Register the ImGui INI settings handler.
		// Must be called before the first ImGui frame (i.e. from Application::Initialize).
		//
		static void RegisterSettingsHandler();
		//
		// Get / set the executable path for a named emulator ("WinAPE", "RVM", "ACE-DL").
		// Returns an empty string for unknown names.
		//
		static const std::string& GetExePath(const char* emulator);
		static void SetExePath(const char* emulator, const std::string& path);
		//
		// Open the ImGui file dialog to browse for an emulator executable.
		// Call each frame: returns true when the user has confirmed a selection,
		// at which point GetExePath() already holds the new path.
		// key must be unique and stable per emulator (use a string literal).
		//
		static void BrowseForExe(const char* emulator);
		static void PollFileDialog();

	private:
		//
		// Stored paths — indexed by emulator name
		//
		inline static std::string s_winapeExe;
		inline static std::string s_rvmExe;
		inline static std::string s_acedlExe;
		//
		// The emulator name whose dialog is currently open (empty = none)
		//
		inline static std::string s_pendingEmulator;
	};

} // namespace RetrodevGui
