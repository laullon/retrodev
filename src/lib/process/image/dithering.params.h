// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- dithering parameter types.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

namespace RetrodevLib {
	struct DitheringParams {
		//
		// Dithering method to use
		//
		std::string Method;
		//
		// Percentage of dithering to be applied (0..400)
		//
		int Percentage;
		//
		// If we should activate error diffusion
		//
		bool ErrorDiffusion;
		//
		// If we should paint pattern matrix on image
		//
		bool Pattern;
		//
		// Pattern Low: divisor for darker variant (higher value = darker)
		// Used to create the darker color in the alternating pattern
		// Typical range: 1.0 (no darkening) to 4.0 (significant darkening)
		//
		float PatternLow;
		//
		// Pattern High: divisor for lighter variant (lower value = brighter)
		// Used to create the lighter color in the alternating pattern
		// Typical range: 1.0 (original brightness) to 4.0 (darker)
		//
		float PatternHigh;

		//
		// Default parameters for new file
		//
		DitheringParams();
	};

}
