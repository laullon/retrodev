// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- colour correction settings serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <process/image/color.correction.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ColorCorrectionParams> {
	using T = RetrodevLib::ColorCorrectionParams;
	static constexpr auto value = object("ColorCorrectionRed", &T::ColorCorrectionRed, "ColorCorrectionGreen", &T::ColorCorrectionGreen, "ColorCorrectionBlue",
										 &T::ColorCorrectionBlue, "Contrast", &T::Contrast, "Saturation", &T::Saturation, "Brightness", &T::Brightness, "ColorBits", &T::ColorBits,
										 "PaletteReductionLower", &T::PaletteReductionLower, "PaletteReductionUpper", &T::PaletteReductionUpper);
};