// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Sprite list widget -- thumbnail grid of extracted sprites.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <convert/convert.sprites.h>
#include <convert/convert.sprites.params.h>
#include <memory>
#include <vector>

namespace RetrodevGui {

	struct SpriteListWidgetResult {
		//
		// Selection change: newSelection holds the full set; primaryIndex is the last clicked sprite
		//
		bool selectionChanged = false;
		std::vector<int> newSelection;
		int primaryIndex = -1;
		//
		// Source indices for all context-menu operations (the selection at menu-open time)
		//
		std::vector<int> operationSourceIndices;
		//
		// Duplicate only (no transform)
		//
		bool duplicate = false;
		//
		// Duplicate with flip
		//
		bool duplicateFlipH = false;
		bool duplicateFlipV = false;
		//
		// Duplicate with shift
		//
		bool duplicateShiftLeft = false;
		bool duplicateShiftRight = false;
		bool duplicateShiftUp = false;
		bool duplicateShiftDown = false;
		//
		// In-place flip (modifies the existing sprites)
		//
		bool flipH = false;
		bool flipV = false;
		//
		// In-place shift (modifies the existing sprites)
		//
		bool shiftLeft = false;
		bool shiftRight = false;
		bool shiftUp = false;
		bool shiftDown = false;
		//
		// Remove the sprites in operationSourceIndices
		//
		bool removeSprites = false;
	};

	class SpriteListWidget {
	public:
		//
		// Render the sprite list UI
		// Shows all extracted sprites in a wrapped grid layout
		// Returns result indicating selection changes and any context-menu operations
		//
		static SpriteListWidgetResult Render(std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor, RetrodevLib::SpriteExtractionParams* spriteParams,
											 const std::vector<int>& selectedIndices, int primaryIndex);
	private:
		//
		// Persistent selection state
		//
		static std::vector<int> m_selectedIndices;
		static int m_lastClickedIndex;
	};
}
