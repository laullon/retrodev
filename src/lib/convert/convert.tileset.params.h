// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tileset conversion parameters.
//
// (c) TLOTB 2026
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
		//
		// Pack-to-Grid pre-processing settings
		// When PackEnabled is true, PackToGrid() is called on the converted image
		// before tile extraction, producing a uniform grid from scattered regions.
		//
		bool PackEnabled = false;
		//
		// Background colour used by the region detector (normalised 0..1 per channel)
		//
		float PackBgR = 1.0f;
		float PackBgG = 0.0f;
		float PackBgB = 1.0f;
		//
		// Per-channel absolute tolerance for background matching (0 = exact)
		//
		int PackBgTolerance = 0;
		//
		// Pixel gap between adjacent detected regions that are merged into one bounding box
		//
		int PackMergeGap = 0;
		//
		// Pixel gap between cells in the packed output image
		//
		int PackCellPadding = 0;
		//
		// Number of columns in the packed grid (0 = automatic: ceil(sqrt(N)))
		//
		int PackColumns = 0;
	};
}
