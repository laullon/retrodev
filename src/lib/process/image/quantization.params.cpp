// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- quantization parameter serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "quantization.params.h"

namespace RetrodevLib {

	QuantizationParams::QuantizationParams() {
		Smoothness = false;
		SortPalette = false;
		ReductionType = ReductionMethod::HigherFrequencies;
		ReductionTime = false;
		UseSourcePalette = false;
	}
}