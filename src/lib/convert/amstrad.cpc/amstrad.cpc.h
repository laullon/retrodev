// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC converter -- system registration and shared constants.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <string>

//
// Define constants to be used in the CPC system implementation
//
namespace RetrodevLib::ConverterAmstradCPC {

	//
	// Target modes for this converter
	//
	namespace CPCModes {
		const std::string Mode0 = "Mode 0";
		const std::string Mode1 = "Mode 1";
		const std::string Mode2 = "Mode 2";
	}
	//
	// List of all available modes
	//
	static const std::vector<std::string> CPCModesList = {CPCModes::Mode0, CPCModes::Mode1, CPCModes::Mode2};
	//
	// Predefined resolutions for this converter
	//
	namespace CPCResolutions {
		const std::string Normal = "Normal";
		const std::string Overscan = "Overscan";
		const std::string Custom = "Custom";
		const std::string Original = "Original";
	}
	//
	// List of all available resolutions
	//
	static const std::vector<std::string> CPCResolutionsList = {CPCResolutions::Normal, CPCResolutions::Overscan, CPCResolutions::Custom, CPCResolutions::Original};
	//
	// Palette types for this system
	//
	namespace CPCPaletteTypes {
		const std::string Hardware = "GA Palette";
		const std::string Plus = "ASIC Palette";
	}
	//
	// List of all available palette types
	//
	static const std::vector<std::string> CPCPaletteTypesList = {CPCPaletteTypes::Hardware, CPCPaletteTypes::Plus};

}