// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- dithering parameter serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

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
}