// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Preview controls widget -- aspect and scanlines display toggles.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <memory>

namespace RetrodevGui {

	//
	// Preview controls widget (aspect correction, scale factor, scanlines)
	// Shared between bitmap, tiles, sprites documents
	//
	class PreviewControlsWidget {
	public:
		//
		// Render preview controls (compact single line)
		// Returns true if any setting changed
		//
		static bool Render(bool* aspectCorrection, int* scaleFactor, bool* scanlines, std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params);
		//
		// Render simplified preview controls without conversion trigger
		// Used for sprite/tile preview where conversion is not needed
		// Returns true if any setting changed
		//
		static bool RenderSimple(bool* aspectCorrection, bool* scanlines);
	};

}
