// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>

namespace RetrodevGui {

	//
	// Palette viewer/editor for standalone image documents (INDEX8 sources).
	// Unlike PaletteWidget, this operates directly on the Image surface palette
	// rather than through an IPaletteConverter.  For RGBA32 images nothing is
	// rendered (the caller should check IsPaletized() before calling Render).
	//
	class ImagePaletteWidget {
	public:
		//
		// Render the palette panel inside the current window/child.
		// Returns true when the image palette was modified and the texture
		// should be refreshed.
		//
		static bool Render(std::shared_ptr<RetrodevLib::Image> image);
		//
		// Returns the palette index of the entry most recently clicked by the user.
		// The image document uses this to drive the active painting color.
		//
		static int GetSelectedPen() { return m_selectedPen; }

	private:
		//
		// Index of the pen currently being edited (-1 = none)
		//
		static int m_selectedPen;
		//
		// Pending edit color components (driven by the RGB sliders in the popup)
		//
		static int m_editR;
		static int m_editG;
		static int m_editB;
		//
		// Controls whether the color editor popup is open
		//
		static bool m_showColorEditor;
		//
		// Render the color editor modal popup
		//
		static bool RenderColorEditorPopup(std::shared_ptr<RetrodevLib::Image> image);
	};

} // namespace RetrodevGui
