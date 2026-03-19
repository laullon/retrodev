// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <process/image/dithering.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::DitheringParams> {
	using T = RetrodevLib::DitheringParams;
	static constexpr auto value = object("Method", &T::Method, "Percentage", &T::Percentage, "ErrorDiffusion", &T::ErrorDiffusion, "Pattern", &T::Pattern, "PatternLow",
										 &T::PatternLow, "PatternHigh", &T::PatternHigh);
};