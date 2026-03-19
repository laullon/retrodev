// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <widgets/image.viewer.widget.h>
#include <widgets/data.export.widget.h>
#include <retrodev.gui.h>

#include <string>
#include <memory>

struct SDL_Texture;

namespace RetrodevGui {

	class DocumentBitmap : public DocumentView {
	public:
		DocumentBitmap(const std::string& name, const std::string& filepath);
		~DocumentBitmap() override;

		// Render the image document content
		//
		void Perform() override;
		//
		// Re-run conversion when external palette changes affect this build item
		//
		void OnProjectItemChanged(const std::string& itemName) override;

	private:
		// Original Image we will convert
		//
		std::shared_ptr<RetrodevLib::Image> m_image;
		//
		// Image viewer instance
		//
		std::shared_ptr<RetrodevGui::ImageViewerWidget> m_imageViewer;

		// Converter to use
		//
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_converter;
		//
		// Export widget instance — owns its own path buffers and ImGui ID scope
		//
		DataExportWidget m_exportWidget;
		//
		// Previous preview image (kept alive to prevent texture blinking)
		// This holds the old preview while a new one is being generated
		//
		std::shared_ptr<RetrodevLib::Image> m_previousPreview;
		//
		// Preview display settings (UI-only, not saved in project)
		//
		bool m_previewAspectCorrection = true;
		//int m_previewScaleFactor = 1;
		bool m_previewScanlines = false;

		// Image state for the left (original)
		//
		ImGui::ZoomableState m_zoomStateLeft;

		// Image state for the right (converted)
		//
		ImGui::ZoomableState m_zoomStateRight;

		// Splitter sizes
		bool m_sizesInitialized = false;
		float m_hSizeLeft = 0;
		float m_hSizeRight = 0;
		float m_vSizeTop = 0;
		float m_vSizeBottom = 0;
		float m_imgSizeLeft = 0;
		float m_imgSizeRight = 0;
	};

} // namespace RetrodevGui
