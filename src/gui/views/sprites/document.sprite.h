// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Sprite extraction document -- define and preview sprite regions.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <widgets/image.viewer.widget.h>
#include <retrodev.gui.h>
#include <convert/convert.sprites.h>
#include <widgets/data.export.widget.h>

#include <string>
#include <memory>
#include <vector>

struct SDL_Texture;

namespace RetrodevGui {

	class DocumentSprite : public DocumentView {
	public:
		DocumentSprite(const std::string& name, const std::string& filepath);
		~DocumentSprite() override;
		//
		// Render the sprite document content
		//
		void Perform() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Sprite; }
		//
		// Re-run conversion when external palette changes affect this build item
		//
		void OnProjectItemChanged(const std::string& itemName) override;

	private:
		//
		// Tab selection (0 = Conversion, 1 = Sprite Extraction)
		//
		int m_activeTab = 0;
		//
		// Image viewer instances (one per tab to maintain independent state)
		//
		std::shared_ptr<RetrodevGui::ImageViewerWidget> m_conversionViewer;
		std::shared_ptr<RetrodevGui::ImageViewerWidget> m_extractionViewer;
		//
		// Original Image we will convert
		//
		std::shared_ptr<RetrodevLib::Image> m_image;
		//
		// Converter to use
		//
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_converter;
		//
		// Sprite extractor
		//
		std::shared_ptr<RetrodevLib::ISpriteExtractor> m_spriteExtractor;
		//
		// Export widget instance -- owns its own path buffers and ImGui ID scope
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
		// int m_previewScaleFactor = 1;
		bool m_previewScanlines = false;
		//
		// Sprite preview display settings (UI-only, for right panel)
		//
		bool m_spritePreviewAspectCorrection = false;
		bool m_spritePreviewScanlines = false;
		//
		// Conversion tab zoom states
		//
		ImGui::ZoomableState m_conversionZoomStateLeft;
		ImGui::ZoomableState m_conversionZoomStateRight;
		//
		// Extraction tab zoom states
		//
		ImGui::ZoomableState m_extractionZoomStateLeft;
		ImGui::ZoomableState m_extractionZoomStateRight;
		//
		// Splitter sizes
		//
		bool m_conversionSizesInitialized = false;
		bool m_extractionSizesInitialized = false;
		float m_hSizeLeft = 0;
		float m_hSizeRight = 0;
		float m_vSizeTop = 0;
		float m_vSizeBottom = 0;
		float m_imgSizeLeft = 0;
		float m_imgSizeRight = 0;
		//
		// Extraction tab vertical splitter sizes
		//
		float m_extractionVSizeTop = 0;
		float m_extractionVSizeBottom = 0;
		//
		// Selected sprite tracking -- primary index drives the tooling panel; selection holds the full multi-select set
		//
		int m_selectedSpriteIndex = -1;
		std::vector<int> m_spriteSelection;
		int m_spritePrimaryIndex = -1;
		std::shared_ptr<RetrodevLib::Image> m_selectedSpriteImage;
		// Cached generated preview for the selected sprite (keeps texture alive)
		std::shared_ptr<RetrodevLib::Image> m_selectedSpritePreview;
		// Cache to detect when preview needs regeneration
		int m_cachedSpritePreviewIndex = -1;
		bool m_cachedSpritePreviewAspect = false;
		bool m_cachedSpritePreviewScanlines = false;
		// Cached transparency flag -- set when preview is regenerated, drives background overlay
		bool m_cachedSpriteHasTransparency = false;
		//
		// Render methods for different tabs
		//
		void RenderConversionTab();
		void RenderSpriteExtractionTab();
	};

}
