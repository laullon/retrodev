// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Shared conversion parameter definitions.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

namespace RetrodevLib {
	//
	// Parameters not processed by the common methods but for the converter itself
	//
	struct ConvertParams {
		//
		// Target system: Used by the converter to instantiate the appropiate converters
		//
		std::string TargetSystem;
		//
		// Target Mode: Used by the converter to match the desired system video mode
		//
		std::string TargetMode;
		//
		// Target Palette Type: Used by the converter to match the desired system palette
		//
		std::string PaletteType;
		//
		// Color Selection Mode: Method used by the palette to select colors
		//
		std::string ColorSelectionMode;
		//
		// Palette lock state per pen -- one entry per active pen
		// Locked pens can be used but their color cannot be changed during quantization
		// Sized to PaletteMaxColors() by the converter on first use
		//
		std::vector<bool> PaletteLocked;
		//
		// Palette enable state per pen -- one entry per active pen
		// Disabled pens cannot be used at all during quantization
		// Sized to PaletteMaxColors() by the converter on first use
		//
		std::vector<bool> PaletteEnabled;
		//
		// Palette color per pen (system color index) -- one entry per active pen
		// -1 means not set / use palette default
		// Locked pens with a valid index here have their color restored on palette construction
		// Sized to PaletteMaxColors() by the converter on first use
		//
		std::vector<int> PaletteColors;

		//
		// Default constructor -- arrays start empty and are sized by the converter
		//
		ConvertParams() : TargetSystem(""), TargetMode(""), PaletteType(""), ColorSelectionMode("") {}
	};
}
