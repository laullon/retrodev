//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "tile.extraction.widget.h"

namespace RetrodevGui {
	//
	// Static member definitions
	//
	bool TileExtractionWidget::m_extractionOpen = true;
	//
	// Main render function for the tile extraction widget
	//
	TileExtractionWidgetResult TileExtractionWidget::Render(RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
															std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params) {
		TileExtractionWidgetResult result;
		if (!tileParams)
			return result;
		bool changed = false;
		bool gridChanged = false;
		//
		// Tile Extraction section
		//
		ImGui::SetNextItemOpen(m_extractionOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Tile Extraction")) {
			m_extractionOpen = true;
			//
			// Tile size settings
			//
			ImGui::Text("Tile Size:");
			if (ImGui::InputInt("Width##TileWidth", &tileParams->TileWidth)) {
				if (tileParams->TileWidth < 1)
					tileParams->TileWidth = 1;
				changed = true;
				gridChanged = true;
			}
			if (ImGui::InputInt("Height##TileHeight", &tileParams->TileHeight)) {
				if (tileParams->TileHeight < 1)
					tileParams->TileHeight = 1;
				changed = true;
				gridChanged = true;
			}
			ImGui::Separator();
			//
			// Offset settings
			//
			ImGui::Text("Starting Offset:");
			if (ImGui::InputInt("Offset X##OffsetX", &tileParams->OffsetX)) {
				if (tileParams->OffsetX < 0)
					tileParams->OffsetX = 0;
				changed = true;
				gridChanged = true;
			}
			if (ImGui::InputInt("Offset Y##OffsetY", &tileParams->OffsetY)) {
				if (tileParams->OffsetY < 0)
					tileParams->OffsetY = 0;
				changed = true;
				gridChanged = true;
			}
			ImGui::Separator();
			//
			// Padding settings
			//
			ImGui::Text("Grid Padding:");
			if (ImGui::InputInt("Padding X##PaddingX", &tileParams->PaddingX)) {
				if (tileParams->PaddingX < 0)
					tileParams->PaddingX = 0;
				changed = true;
				gridChanged = true;
			}
			if (ImGui::InputInt("Padding Y##PaddingY", &tileParams->PaddingY)) {
				if (tileParams->PaddingY < 0)
					tileParams->PaddingY = 0;
				changed = true;
				gridChanged = true;
			}
			ImGui::Separator();
			//
			// Display tile extraction info
			//
			if (tileExtractor && converter) {
				int tileCount = tileExtractor->GetTileCount();
				ImGui::Text("Tiles Extracted: %d", tileCount);
				//
				// Calculate grid dimensions using native converted image
				//
				if (params) {
					auto convertedImage = converter->GetConverted(params);
					if (convertedImage) {
						int imageWidth = convertedImage->GetWidth();
						int imageHeight = convertedImage->GetHeight();
						int availableWidth = imageWidth - tileParams->OffsetX;
						int availableHeight = imageHeight - tileParams->OffsetY;
						if (availableWidth >= tileParams->TileWidth && availableHeight >= tileParams->TileHeight) {
							int tilesX = 1 + ((availableWidth - tileParams->TileWidth) / (tileParams->TileWidth + tileParams->PaddingX));
							int tilesY = 1 + ((availableHeight - tileParams->TileHeight) / (tileParams->TileHeight + tileParams->PaddingY));
							ImGui::Text("Grid Size: %dx%d", tilesX, tilesY);
						}
					}
				}
			}
			ImGui::Separator();
			//
			// Extract button
			//
			if (ImGui::Button("Extract Tiles", ImVec2(-1, 0))) {
				result.extractRequested = true;
			}
		} else {
			m_extractionOpen = false;
		}
		result.parametersChanged = changed;
		result.gridStructureChanged = gridChanged;
		return result;
	}
} // namespace RetrodevGui
