// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Image viewer and pixel paint editor document.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <widgets/image.palette.widget.h>
#include <widgets/painting.widget.h>

#include <string>
#include <memory>
#include <vector>

struct SDL_Texture;

namespace RetrodevGui {

	class DocumentImage : public DocumentView {
	public:
		DocumentImage(const std::string& name, const std::string& filepath);
		~DocumentImage() override;

		// Render the image document content
		void Perform() override;
		// Save the image to disk
		bool Save() override;

	private:
		std::shared_ptr<RetrodevLib::Image> m_image;

		ImGui::ZoomableState m_zoomState;

		// True when the palette was edited and the texture needs re-upload
		// bool m_paletteChanged = false;
		// Palette index currently selected for painting (driven by ImagePaletteWidget)
		int m_activePen = 0;
		// Drag state for shape tools (line, rect, ellipse)
		bool m_isDragging = false;
		ImVec2 m_dragStart = ImVec2(0.0f, 0.0f);
		int m_dragPenIndex = 0;
		// Selection stamp buffer: pixels captured from the selected region
		// Stored as palette indices (paletized) or packed RGBA (non-paletized)
		// m_stampRect: x, y, w, h in image pixel coordinates
		std::vector<int> m_stampBuffer;
		ImVec4 m_stampRect = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
		bool m_hasStamp = false;
		// Undo/redo stacks -- each entry is a full pixel snapshot
		// Paletized: palette indices; RGBA: packed 0xRRGGBB per pixel
		static constexpr int k_maxUndoSteps = 50;
		std::vector<std::vector<int>> m_undoStack;
		std::vector<std::vector<int>> m_redoStack;
		// Splitter sizes
		bool m_sizesInitialized = false;
		float m_hSizeLeft = 0;
		float m_hSizeRight = 0;
		float m_vSizeTop = 0;
		float m_vSizeBottom = 0;
		//
		// Capture a full pixel snapshot and push it onto the undo stack
		//
		void CaptureSnapshot();
		//
		// Restore the image from a pixel snapshot
		//
		void ApplySnapshot(const std::vector<int>& snapshot);
	};

}