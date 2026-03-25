// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- emulator launch configuration.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "source.h"

namespace RetrodevLib {

	//
	// SourceEmulator -- launches the configured emulator for a build item.
	// Reads EmulatorParams from the supplied SourceParams, builds the correct
	// command line for the selected emulator (WinAPE / RVM / ACE-DL) and
	// spawns the process.  All diagnostic messages are forwarded through logCb.
	//
	class SourceEmulator {
	public:
		//
		// Launch the emulator described in params->emulatorParams.
		// Returns true if the process was started successfully, false on error.
		// Does nothing (returns true) when emulatorParams.emulator is empty.
		// All diagnostics are emitted via Log on LogChannel::Build.
		//
		static bool Launch(const SourceParams* params);
		//
		// Poll all running emulator processes: drain stdout/stderr into Log on LogChannel::Build
		// and destroy processes that have exited.  Call once per frame.
		//
		static void Poll();
	};

}
