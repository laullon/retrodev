// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- per-channel colour correction (brightness, contrast, saturation).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include "color.correction.params.h"
#include <assets/image/image.color.h>
#include <cstdint>
#include <vector>

namespace RetrodevLib {

	class GFXColor {
	public:
		//
		// Contrast table
		//
		static std::vector<uint8_t> tblContrast;
		//
		// Precalculated Color cap low & high
		//
		static int m1;
		static int m2;

		//
		// Initializes the contrast table (256 levels of contrast)
		//
		static void ContrastInitialize(const ColorCorrectionParams& prms);
		//
		// Apply a color correction against the given color using the color transform parameters
		//
		static RgbColor ApplyColorCorrection(const ColorCorrectionParams& prm, RgbColor p);
		//
		// Get list of available palette reduction lower limits for UI combo box
		//
		static std::vector<std::string> GetPaletteReductionLowerLimits();
		//
		// Get list of available palette reduction upper limits for UI combo box
		//
		static std::vector<std::string> GetPaletteReductionUpperLimits();

	private:
		//
		// Set luminosity and saturation on the given RGB components
		//
		static void SetLumiSat(float lumi, float satur, float& r, float& g, float& b);
	};

}