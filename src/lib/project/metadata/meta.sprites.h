// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.sprites.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for SpriteDefinition serialization
//
template <> struct glz::meta<RetrodevLib::SpriteDefinition> {
	using T = RetrodevLib::SpriteDefinition;
	static constexpr auto value = object("X", &T::X, "Y", &T::Y, "Width", &T::Width, "Height", &T::Height, "Name", &T::Name);
};
//
// Glaze metadata for SpriteExtractionParams serialization
//
template <> struct glz::meta<RetrodevLib::SpriteExtractionParams> {
	using T = RetrodevLib::SpriteExtractionParams;
	static constexpr auto value = object("Sprites", &T::Sprites);
};
