// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- image asset serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.bitmap.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::GFXParams> {
	using T = RetrodevLib::GFXParams;
	static constexpr auto value = object("SParams", &T::SParams, "RParams", &T::RParams, "DParams", &T::DParams, "QParams", &T::QParams, "CParams", &T::CParams);
};