// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- platform-specific emulator process launch interface.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "source.h"
#include <string>

namespace RetrodevLib {

	//
	// SourceEmulatorNative -- platform-specific process launch and polling.
	// Each platform supplies one implementation file; the build system
	// includes only the file matching the current target OS.
	//
	// Process handles are exposed as opaque void* so no platform headers or
	// ifdefs are needed outside the per-platform implementation files.
	// Each implementation casts the void* to its own OS handle type internally.
	//
	class SourceEmulatorNative {
	public:
		//
		// Launch a GUI process using the native OS API with no stdio redirection.
		// rawCmdLine is passed verbatim to the OS (no re-quoting).
		// workDir sets the working directory; pass empty to use the exe directory.
		// Returns a non-null opaque handle on success, nullptr on failure.
		// Diagnostics are emitted via Log on LogChannel::Build.
		//
		static void* Spawn(const std::string& exePath, const std::string& rawCmdLine, const std::string& workDir);
		//
		// Non-blocking exit check.
		// Returns true when the process has exited and writes its exit code to outExitCode.
		// Returns false when the process is still running.
		// handle must be a value previously returned by Spawn.
		//
		static bool Poll(void* handle, int& outExitCode);
		//
		// Release OS resources associated with the handle.
		// Must be called exactly once after Poll returns true.
		//
		static void Destroy(void* handle);
	};

}
