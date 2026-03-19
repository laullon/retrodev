// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <convert/convert.sprites.h>
#include <convert/convert.sprites.params.h>
#include <memory>

namespace RetrodevGui {

	struct SpriteExtractionWidgetResult {
		bool parametersChanged = false;
		bool extractionTriggered = false;
		bool addSpriteRequested = false;
		bool selectionModeActive = false;
	};

	class SpriteExtractionWidget {
	public:
		//
		// Render the sprite extraction parameters UI
		// Returns result indicating if parameters changed or actions were triggered
		//
		static SpriteExtractionWidgetResult Render(RetrodevLib::SpriteExtractionParams* spriteParams, std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor,
												   int selectedSpriteIndex);
		//
		// Check if selection mode is active
		//
		static bool IsSelectionModeActive();
		//
		// Set selection mode state
		//
		static void SetSelectionMode(bool active);
		//
		// Get the current selection rectangle
		//
		static void GetSelection(int& x, int& y, int& width, int& height);
		//
		// Set the selection rectangle (used during mouse dragging)
		//
		static void SetSelection(int x, int y, int width, int height);
		//
		// Confirm the current selection (adds sprite to params)
		//
		static void ConfirmSelection();
		//
		// Get the pending sprite name
		//
		static std::string GetPendingSpriteName();

	private:
		static bool m_selectionModeActive;
		static int m_selectionX;
		static int m_selectionY;
		static int m_selectionWidth;
		static int m_selectionHeight;
		static std::string m_pendingSpriteName;
	};
} // namespace RetrodevGui
