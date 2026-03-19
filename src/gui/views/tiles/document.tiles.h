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
#include <convert/convert.tileset.h>

#include <string>
#include <memory>

struct SDL_Texture;

namespace RetrodevGui {

	class DocumentTiles : public DocumentView {
	public:
		DocumentTiles(const std::string& name, const std::string& filepath);
		~DocumentTiles() override;
		//
		// Render the tiles document content
		//
		void Perform() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Tilemap; }
		//
		// Re-run conversion when external palette changes affect this build item
		//
		void OnProjectItemChanged(const std::string& itemName) override;

	private:
		//
		// Tab selection (0 = Conversion, 1 = Tile Extraction)
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
		// Tile extractor
		//
		std::shared_ptr<RetrodevLib::ITileExtractor> m_tileExtractor;
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
		// Selected tile tracking
		//
		int m_selectedTileIndex = -1;
		std::shared_ptr<RetrodevLib::Image> m_selectedTileImage;
		// Cached generated preview for the selected tile (keeps texture alive)
		std::shared_ptr<RetrodevLib::Image> m_selectedTilePreview;
		// Cache to detect when tile preview needs regeneration
		int m_cachedTilePreviewIndex = -1;
		bool m_cachedTilePreviewAspect = false;
		bool m_cachedTilePreviewScanlines = false;
		//
		// Tile preview display settings (UI-only, for right panel)
		//
		bool m_tilePreviewAspectCorrection = false;
		bool m_tilePreviewScanlines = false;
		//
		// Tile extraction settings (visual only for now)
		//
		// int m_tileWidth = 8;
		// int m_tileHeight = 8;
		// bool m_showGrid = true;
		// int m_selectedTileX = 0;
		// int m_selectedTileY = 0;
		//
		// Script-driven export widget (owned per-document to isolate UI state)
		//
		DataExportWidget m_exportWidget;
		//
		// Render methods for different tabs
		//
		void RenderConversionTab();
		void RenderTileExtractionTab();
	};

} 
