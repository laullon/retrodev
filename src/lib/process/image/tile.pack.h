// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tile pack-to-grid -- detects content regions and rearranges them into a regular grid.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <memory>
#include <vector>

namespace RetrodevLib {
	//
	// A single detected content region (bounding box in the source image)
	//
	struct TilePackRegion {
		int x, y, w, h;
	};
	//
	// Result returned by PackToGrid
	//
	struct TilePackResult {
		//
		// The rearranged output image; nullptr if no regions were found
		//
		std::shared_ptr<Image> packedImage;
		//
		// Cell dimensions used in the packed image
		//
		int cellWidth = 0;
		int cellHeight = 0;
		//
		// Padding between cells in the packed image
		//
		int cellPadding = 0;
		//
		// Number of regions detected and packed
		//
		int regionCount = 0;
	};
	//
	// Detect content bounding boxes in sourceImage and pack them into a new image
	// arranged on a uniform grid.
	//
	// bgColor       -- background / separator colour to ignore
	// bgTolerance   -- per-channel absolute tolerance (0 = exact match)
	// mergeGap      -- pixel gap between adjacent regions that should be merged (0 = no merging)
	// cellPadding   -- pixel gap between cells in the output packed image
	// gridColumns   -- number of columns in the output grid; 0 = auto (ceil(sqrt(N)))
	//
	TilePackResult PackToGrid(std::shared_ptr<Image> sourceImage, RgbColor bgColor, int bgTolerance, int mergeGap, int cellPadding, int gridColumns);
}
