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

	struct SpriteListWidgetResult {
		bool spriteSelected = false;
		int selectedSpriteIndex = -1;
	};

	class SpriteListWidget {
	public:
		//
		// Render the sprite list UI
		// Shows all extracted sprites in a wrapped grid layout
		// Returns result indicating if a sprite was selected
		//
		static SpriteListWidgetResult Render(std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor, RetrodevLib::SpriteExtractionParams* spriteParams,
											 int currentSelectedIndex);
	};
} // namespace RetrodevGui
