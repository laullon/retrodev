// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Build document -- emulator launch settings panel.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "document.build.settings.h"
#include <retrodev.gui.h>
#include <imgui_internal.h>
#include <cstdio>

namespace RetrodevGui {

	namespace EmulatorSettingsImpl {

		//
		// Dialog key prefix -- each emulator gets its own unique key
		//
		static constexpr const char* k_dialogKey = "EmulatorExeBrowse";
		//
		// INI read-open: return a non-null sentinel so ReadLine is called
		//
		static void* IniReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*) {
			return (void*)1;
		}
		//
		// INI read-line: parse "WinAPE=...", "RVM=...", "ACEDL=..."
		//
		static void IniReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
			char buf[512];
			if (sscanf(line, "WinAPE=%511[^\n]", buf) == 1)
				EmulatorSettings::SetExePath("WinAPE", buf);
			else if (sscanf(line, "RVM=%511[^\n]", buf) == 1)
				EmulatorSettings::SetExePath("RVM", buf);
			else if (sscanf(line, "ACEDL=%511[^\n]", buf) == 1)
				EmulatorSettings::SetExePath("ACE-DL", buf);
		}
		//
		// INI write-all: emit all three paths (even if empty, to preserve the section)
		//
		static void IniWriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
			buf->appendf("[%s][Emulators]\n", handler->TypeName);
			buf->appendf("WinAPE=%s\n", EmulatorSettings::GetExePath("WinAPE").c_str());
			buf->appendf("RVM=%s\n", EmulatorSettings::GetExePath("RVM").c_str());
			buf->appendf("ACEDL=%s\n", EmulatorSettings::GetExePath("ACE-DL").c_str());
			buf->append("\n");
		}

	}

	//
	// Register the INI handler -- must be called before the first ImGui frame
	//
	void EmulatorSettings::RegisterSettingsHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "EmulatorPaths";
		handler.TypeHash = ImHashStr("EmulatorPaths");
		handler.ReadOpenFn = EmulatorSettingsImpl::IniReadOpen;
		handler.ReadLineFn = EmulatorSettingsImpl::IniReadLine;
		handler.WriteAllFn = EmulatorSettingsImpl::IniWriteAll;
		ImGui::AddSettingsHandler(&handler);
	}

	const std::string& EmulatorSettings::GetExePath(const char* emulator) {
		if (emulator == nullptr)
			return s_winapeExe;
		if (SDL_strcmp(emulator, "WinAPE") == 0)
			return s_winapeExe;
		if (SDL_strcmp(emulator, "RVM") == 0)
			return s_rvmExe;
		if (SDL_strcmp(emulator, "ACE-DL") == 0)
			return s_acedlExe;
		return s_winapeExe;
	}

	void EmulatorSettings::SetExePath(const char* emulator, const std::string& path) {
		if (emulator == nullptr)
			return;
		if (SDL_strcmp(emulator, "WinAPE") == 0)
			s_winapeExe = path;
		else if (SDL_strcmp(emulator, "RVM") == 0)
			s_rvmExe = path;
		else if (SDL_strcmp(emulator, "ACE-DL") == 0)
			s_acedlExe = path;
	}

	void EmulatorSettings::BrowseForExe(const char* emulator) {
		s_pendingEmulator = emulator;
		//
		// Open the ImGui file dialog; use the current exe directory as the starting location
		//
		const std::string& current = GetExePath(emulator);
		std::string startDir;
		if (!current.empty())
			startDir = std::filesystem::path(current).parent_path().string();
		ImGui::FileDialog::Instance().Open(EmulatorSettingsImpl::k_dialogKey, std::string("Select ") + emulator + " executable",
										   "Executable files (*.exe){.exe},All files (*.*){.*}", false, startDir);
	}

	void EmulatorSettings::PollFileDialog() {
		if (s_pendingEmulator.empty())
			return;
		if (!ImGui::FileDialog::Instance().IsDone(EmulatorSettingsImpl::k_dialogKey))
			return;
		if (ImGui::FileDialog::Instance().HasResult()) {
			SetExePath(s_pendingEmulator.c_str(), ImGui::FileDialog::Instance().GetResult().string());
			ImGui::MarkIniSettingsDirty();
		}
		ImGui::FileDialog::Instance().Close();
		s_pendingEmulator.clear();
	}

}
