//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.lib.h>
#include <memory>

namespace RetrodevGui {
	//
	// Tile list widget - displays extracted tiles in a grid layout
	// Allows selection and interaction with individual tiles
	//
	class TileListWidget {
	public:
		//
		// Render result structure
		//
		struct RenderResult {
			bool tileSelected = false;
			int selectedTileIndex = -1;
			bool tileDeleted = false;
			int deletedTileIndex = -1;
		};
		//
		// Render the tile list widget
		// Returns result with selection and deletion information
		// tileExtractor: The tile extractor containing extracted tiles
		// tileParams: Parameters used for extraction (needed to show deleted tiles)
		// imageWidth, imageHeight: Dimensions of the source image (to calculate total grid)
		//
		static RenderResult Render(std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor, const RetrodevLib::TileExtractionParams* tileParams = nullptr, int imageWidth = 0,
								   int imageHeight = 0);
	};
} // namespace RetrodevGui
