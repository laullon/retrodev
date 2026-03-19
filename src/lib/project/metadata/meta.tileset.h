// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.tileset.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for TileExtractionParams JSON serialization
//
template <> struct glz::meta<RetrodevLib::TileExtractionParams> {
	using T = RetrodevLib::TileExtractionParams;
	static constexpr auto value = object("TileWidth", &T::TileWidth, "TileHeight", &T::TileHeight, "OffsetX", &T::OffsetX, "OffsetY", &T::OffsetY, "PaddingX", &T::PaddingX,
										 "PaddingY", &T::PaddingY, "DeletedTiles", &T::DeletedTiles);
};
