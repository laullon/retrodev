// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- converter settings serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
// Note: Vectors (PaletteLocked, PaletteEnabled, PaletteColors) serialize as JSON arrays
// Use glz::write<glz::opts{.prettify = true}> for readable JSON with compact arrays
//
template <> struct glz::meta<RetrodevLib::ConvertParams> {
	using T = RetrodevLib::ConvertParams;
	static constexpr auto value = object("TargetSystem", &T::TargetSystem, "TargetMode", &T::TargetMode, "PaletteType", &T::PaletteType, "ColorSelectionMode",
										 &T::ColorSelectionMode, "PaletteLocked", &T::PaletteLocked, "PaletteEnabled", &T::PaletteEnabled, "PaletteColors", &T::PaletteColors);
};
