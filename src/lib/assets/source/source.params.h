// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset parameters -- file paths and include directories.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <map>

namespace RetrodevLib {

	//
	// Parameters for a source build item.
	// Holds the tool selection, source file list, include directories,
	// preprocessor defines and per-tool command-line options.
	//
	struct SourceParams {
		//
		// Selected build tool identifier (e.g. "RASM").
		// Empty string means no tool has been explicitly chosen yet.
		//
		std::string tool;
		//
		// Ordered list of source file paths (relative to the project folder)
		// that this build item compiles or processes
		//
		std::vector<std::string> sources;
		//
		// Include search directories passed to the assembler / compiler (-I)
		//
		std::vector<std::string> includeDirs;
		//
		// Preprocessor macro definitions passed before assembly (e.g. "MYFLAG=1")
		//
		std::vector<std::string> defines;
		//
		// Ordered list of build item names (bitmaps, tilesets, sprites, maps, palettes)
		// that must be processed before this build executes.
		// Each entry is the full virtual path of the dependency (e.g. "graphics/cpc/tiles").
		//
		std::vector<std::string> dependencies;
		//
		// Extra command-line arguments for each tool, keyed by tool name (e.g. "RASM").
		// The value is an opaque string of flags that the lib layer appends verbatim
		// when invoking the tool (e.g. "-w -twe -pasmo").
		//
		std::map<std::string, std::string> toolOptions;
		//
		// Emulator launch settings: which emulator to use and its per-emulator options
		//
		struct EmulatorParams {
			//
			// Selected emulator identifier: "WinAPE", "RVM", "ACE-DL", or empty (none)
			//
			std::string emulator;
			//
			// Common fields -- shared by all emulators; the back-end in source.emulator.cpp
			// maps each field to the correct CLI flag for the selected emulator.
			//
			//   mediaFile  -- disc/tape/cartridge/snapshot to insert (DSK, CDT, HFE, XPR, CPR, SNA...)
			//                WinAPE: positional; RVM: -insert <file>; ACE-DL: positional
			//   snapshot   -- snapshot file to load (.sna)
			//                WinAPE: /SN:<file>; RVM: -snapshot <file>; ACE-DL: positional
			//   symbolFile -- assembler/debugger symbol file (.sym)
			//                WinAPE: /SYM:<file>; RVM: not supported; ACE-DL: positional
			//   command    -- program/command to run on startup
			//                WinAPE: /A:<name> (empty = bare /A); RVM: -command=<text>; ACE-DL: -autoRunFile <f>
			//   sendCPM    -- send |CPM command on startup
			//                WinAPE: /A (bare, no name); RVM: -command=|CPM\n; ACE-DL: -autoRunCPM flag
			//   machine    -- machine model to emulate (empty = emulator default)
			//                WinAPE: not used; RVM: -boot=<id>; ACE-DL: not used (reads config)
			//
			struct CommonFields {
				std::string mediaFile;
				std::string snapshot;
				std::string symbolFile;
				std::string command;
				bool sendCPM = false;
				std::string machine;
			} common;
			//
			// WinAPE-specific options.
			// Command line: WinAPE [mediaFile] [/A[:command]|/A] [/SN:snapshot] [/SYM:symbolFile]
			//   exePath -- path to WinAPE.exe
			//
			struct WinApeParams {
				std::string exePath;
			} winape;
			//
			// Retro Virtual Machine v2.0 Beta-1 R7 -- specific options.
			// Command line: RetroVirtualMachine -boot=<machine> [-insert <mediaFile>]
			//               [-snapshot <snapshot>] [-command=<command>]
			//   exePath -- path to RetroVirtualMachine.exe
			//
			struct RvmParams {
				std::string exePath;
			} rvm;
			//
			// ACE-DL -- specific options.
			// Must be launched with CWD set to the ACE-DL executable directory.
			// Command line: AceDL [mediaFile] [symbolFile] [-autoRunFile <command>] [-autoRunCPM]
			//               [-crtc n] [-ram n] [-f*] [-speed n] [-alone] [-skipConfigFile]
			//   exePath        -- path to AceDL.exe (CWD is set to its containing folder at launch)
			//   crtc           -- CRTC type 0-4 (-crtc n), -1 = not set
			//   ram            -- RAM size in KB: 64, 128, 320, 576 (-ram n), 0 = not set
			//   firmware       -- firmware locale: "" (default), "uk", "fr", "sp", "dk"
			//   speed          -- emulation speed 5-200 (-speed n) or -1 for MAX, 0 = not set
			//   alone          -- disable all debug windows, emulator window only (-alone)
			//   skipConfigFile -- do not load/save the configuration file (-skipConfigFile)
			//
			struct AceDlParams {
				std::string exePath;
				int crtc = -1;
				int ram = 0;
				std::string firmware;
				int speed = 0;
				bool alone = false;
				bool skipConfigFile = false;
			} acedl;
		} emulatorParams;
	};

}
