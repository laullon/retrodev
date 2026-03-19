//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "quantization.params.h"

namespace RetrodevLib {

	QuantizationParams::QuantizationParams() {
		Smoothness = false;
		SortPalette = false;
		ReductionType = ReductionMethod::HigherFrequencies;
		ReductionTime = false;
		UseSourcePalette = false;
	}
} // namespace RetrodevLib