// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- colour correction parameter serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "color.correction.params.h"

namespace RetrodevLib {

	ColorCorrectionParams::ColorCorrectionParams() {
		ColorCorrectionBlue = 100;
		ColorCorrectionGreen = 100;
		ColorCorrectionRed = 100;
		Contrast = 100;
		Saturation = 100;
		Brightness = 100;
		ColorBits = 24;
		PaletteReductionLower = PaletteReductionLimits::Lower_None;
		PaletteReductionUpper = PaletteReductionLimits::Upper_None;
	}
}