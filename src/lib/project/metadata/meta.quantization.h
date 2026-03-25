// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- quantization settings serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <process/image/quantization.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::QuantizationParams::ReductionMethod> {
	using enum RetrodevLib::QuantizationParams::ReductionMethod;
	static constexpr auto value = enumerate("HigherFrequencies", HigherFrequencies, "HigherDistances", HigherDistances);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::QuantizationParams> {
	using T = RetrodevLib::QuantizationParams;
	static constexpr auto value = object("Smoothness", &T::Smoothness, "SortPalette", &T::SortPalette, "ReductionMethod", &T::ReductionType, "ReductionTime", &T::ReductionTime,
										 "UseSourcePalette", &T::UseSourcePalette);
};
