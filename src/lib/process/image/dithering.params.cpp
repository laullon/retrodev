//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "dithering.params.h"

namespace RetrodevLib {

	DitheringParams::DitheringParams() {
		Method = "None";
		Percentage = 100;
		ErrorDiffusion = false;
		Pattern = false;
		PatternLow = 1.9f;
		PatternHigh = 2.1f;
	}
} // namespace RetrodevLib