// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>

namespace RetrodevLib {
	//
	// Tile extraction parameters
	// Defines how to extract individual tiles from a converted tileset image
	// These parameters control the grid-based extraction process
	//
	struct TileExtractionParams {
		//
		// Tile dimensions in pixels
		// Defines the width and height of each individual tile
		//
		int TileWidth = 8;
		int TileHeight = 8;
		//
		// Starting offset for extraction (in pixels)
		// Allows skipping a margin at the top-left of the tileset
		// Useful when the tileset has decorative borders or headers
		//
		int OffsetX = 0;
		int OffsetY = 0;
		//
		// Padding/spacing between tiles (in pixels)
		// Useful when tiles are separated by grid lines or borders
		// These pixels are skipped when moving from one tile to the next
		//
		int PaddingX = 0;
		int PaddingY = 0;
		//
		// Deleted tiles (absolute indices in the extraction grid)
		// Tiles marked for deletion will be skipped during extraction
		// Indices are absolute positions in the grid, not display indices
		//
		std::vector<int> DeletedTiles;
	};
} // namespace RetrodevLib
