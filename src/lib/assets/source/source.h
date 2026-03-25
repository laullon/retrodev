// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Source asset -- assembler source file reference.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <project/project.h>
#include <log/log.h>
#include <string>
#include <vector>

namespace RetrodevLib {

	//
	// SourceBuild -- entry point for assembling / compiling a build item.
	// Reads the SourceParams, selects the appropriate back-end tool (currently
	// only RASM), and streams all diagnostics through the Log facility on
	// LogChannel::Build so the host can route them to the Build console tab.
	//
	class SourceBuild {
	public:
		//
		// Assemble all source files listed in params using the tool named in params->tool.
		// Every info, warning and error message is emitted via Log on LogChannel::Build.
		// Returns true if assembly succeeded with no errors, false otherwise.
		//
		static bool Build(const SourceParams* params);
	};

}
