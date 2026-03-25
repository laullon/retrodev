// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Painting widget -- per-pixel paint tool for sprite editing.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>

namespace RetrodevGui {

	//
	// Painting tool types
	//
	enum class PaintingTool { None = 0, Pencil = 1, Eraser = 2, Fill = 3, Line = 4, RectOutline = 5, RectFill = 6, EllipseOutline = 7, EllipseFill = 8, RegionSelect = 9 };
	//
	// Brush shape types
	//
	enum class BrushShape { Square = 0, Circle = 1, Diamond = 2 };
	//
	// Active paint slot: determines which color is applied by drawing tools
	//
	enum class PaintSlot { Paint = 0, Erase = 1 };

	//
	// Pixel painting tools panel for the image document.
	// Color selection is driven externally by ImagePaletteWidget --
	// the active palette index is passed in each frame via Render().
	//
	class PaintingWidget {
	public:
		//
		// Render the tool and brush-size controls.
		// selectedColor: palette index currently active in ImagePaletteWidget.
		// image: used to resolve the active color swatch from the palette.
		// Returns true if the image was modified this frame.
		//
		static bool Render(int selectedColor, std::shared_ptr<RetrodevLib::Image> image);
		//
		// Query current tool/brush state (used by the document when applying paint)
		//
		static PaintingTool GetSelectedTool() { return m_selectedTool; }
		static int GetSelectedColor() { return m_selectedColor; }
		static int GetBrushSize() { return m_brushSize; }
		static BrushShape GetBrushShape() { return m_brushShape; }
		static int GetEraseColor() { return m_eraseColor; }
		static PaintSlot GetActiveSlot() { return m_activeSlot; }
		static int GetFillTolerance() { return m_fillTolerance; }

	private:
		static PaintingTool m_selectedTool;
		static int m_selectedColor;
		static int m_brushSize;
		static BrushShape m_brushShape;
		static int m_eraseColor;
		static PaintSlot m_activeSlot;
		static int m_fillTolerance;
		//
		// Render helpers
		//
		static void RenderToolButtons();
		static void RenderBrushSizeButtons();
	};

}
