// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

namespace RetrodevLib {

	//
	// Export configuration stored per build item.
	// Holds the script path and output file name chosen by the user in the Export section.
	// Both fields are optional — empty means the user has not configured export yet.
	//
	struct ExportParams {
		//
		// Absolute path to the AngelScript export script selected by the user
		//
		std::string scriptPath;
		//
		// Output file name (relative to the project folder) written by the export script
		//
		std::string outputName;
		//
		// Serialised script parameter values: "key=value;key=value;..."
		// Keys and default values are defined by @param tags in the script.
		// The string is opaque to the project layer; the export widget owns its structure.
		//
		std::string scriptParams;
	};

} // namespace RetrodevLib
