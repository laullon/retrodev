// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- quantization parameter types.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>

namespace RetrodevLib {

	//
	// Quantization parameters used by the quantization routines
	//
	struct QuantizationParams {
		//
		// If smoothness should be applied
		//
		bool Smoothness;
		//
		// Sort palette when reducing colors
		//
		bool SortPalette;
		//
		// Reduction method used when reducing the palette
		//
		enum class ReductionMethod { HigherFrequencies, HigherDistances } ReductionType;
		//
		// If the palette reduction is applied before dithering or after.
		//
		bool ReductionTime;
		//
		// When true and the source image is paletized, the source palette entries are
		// copied directly into the active pen slots before quantization runs.
		// This lets artists pre-assign CPC colours and skip quantization fitting.
		//
		bool UseSourcePalette;

		//
		// Default constructor
		//
		QuantizationParams();
	};
}
