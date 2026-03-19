//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

namespace RetrodevLib {
	//
	// Palette reduction limit constants
	//
	namespace PaletteReductionLimits {
		//
		// Lower limit values (OR'd together to set minimum RGB values)
		//
		const std::string Lower_None = "None (0x00)";
		const std::string Lower_0x11 = "0x11";
		const std::string Lower_0x22 = "0x22";
		const std::string Lower_0x33 = "0x33 (both)";
		//
		// Upper limit values (AND'd together to set maximum RGB values)
		//
		const std::string Upper_None = "None (0xFF)";
		const std::string Upper_0xEE = "0xEE";
		const std::string Upper_0xDD = "0xDD";
		const std::string Upper_0xCC = "0xCC (both)";
	} // namespace PaletteReductionLimits

	struct ColorCorrectionParams {
		//
		// Value for color correction in channel red
		//
		int ColorCorrectionRed;
		//
		// Value for color correction in channel green
		//
		int ColorCorrectionGreen;
		//
		// Value for color correction in channel blue
		//
		int ColorCorrectionBlue;
		//
		// Amount of contrast
		//
		int Contrast;
		//
		// Amount of saturation
		//
		int Saturation;
		//
		// Amount of Brightness
		//
		int Brightness;
		//
		// Specify the amount of bits to be used for each color component.
		// 24/12/9/6
		//
		int ColorBits;
		//
		// Lower limit for palette reduction (OR mask)
		// Values: "None (0x00)", "0x11", "0x22", "0x33 (both)"
		//
		std::string PaletteReductionLower;
		//
		// Upper limit for palette reduction (AND mask)
		// Values: "None (0xFF)", "0xEE", "0xDD", "0xCC (both)"
		//
		std::string PaletteReductionUpper;

		//
		// Default parameters for new file
		//
		ColorCorrectionParams();
	};
} // namespace RetrodevLib
