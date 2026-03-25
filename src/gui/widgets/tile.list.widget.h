// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Tile list widget -- thumbnail grid of extracted tiles.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.lib.h>
#include <memory>
#include <vector>

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
			//
			// Selection change: newSelection holds the full set; primaryIndex is the last clicked tile
			//
			bool selectionChanged = false;
			std::vector<int> newSelection;
			int primaryIndex = -1;
			//
			// Tile toggle (delete/undelete): all indices in toggleIndices are toggled
			//
			bool tileToggled = false;
			std::vector<int> toggleIndices;
		};
		//
		// Render the tile list widget
		// Returns result with selection and deletion information
		// tileExtractor: The tile extractor containing extracted tiles
		// tileParams: Parameters used for extraction (needed to show deleted tiles)
		// imageWidth, imageHeight: Dimensions of the source image (to calculate total grid)
		//
		static RenderResult Render(std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor, const RetrodevLib::TileExtractionParams* tileParams,
								   const std::vector<int>& selectedIndices, int primaryIndex, int imageWidth = 0, int imageHeight = 0);

	private:
		//
		// Persistent selection state
		//
		static std::vector<int> m_selectedIndices;
		static int m_lastClickedIndex;
	};
}
