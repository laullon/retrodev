//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "tile.list.widget.h"
#include <app/app.h>
#include <algorithm>

namespace RetrodevGui {
	//
	// Render the tile list widget
	//
	TileListWidget::RenderResult TileListWidget::Render(std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor, const RetrodevLib::TileExtractionParams* tileParams,
														int imageWidth, int imageHeight) {
		RenderResult result;
		//
		// Check if we have a valid tile extractor
		//
		if (!tileExtractor) {
			ImGui::Text("No tile extractor available");
			return result;
		}
		//
		// Only show tiles if extraction has been performed
		// Don't show the grid if no tiles have been extracted yet
		//
		int extractedCount = tileExtractor->GetTileCount();
		if (extractedCount == 0) {
			ImGui::Text("No tiles extracted yet");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click 'Extract Tiles' to start");
			return result;
		}
		//
		// Calculate total tiles in the extraction grid (including deleted ones)
		// We need to show ALL tile positions, marking deleted ones visually
		//
		int totalGridTiles = 0;
		if (tileParams && imageWidth > 0 && imageHeight > 0) {
			//
			// Calculate grid dimensions from image and parameters
			//
			int availableWidth = imageWidth - tileParams->OffsetX;
			int availableHeight = imageHeight - tileParams->OffsetY;
			if (availableWidth >= tileParams->TileWidth && availableHeight >= tileParams->TileHeight) {
				int tilesX = 1 + ((availableWidth - tileParams->TileWidth) / (tileParams->TileWidth + tileParams->PaddingX));
				int tilesY = 1 + ((availableHeight - tileParams->TileHeight) / (tileParams->TileHeight + tileParams->PaddingY));
				totalGridTiles = tilesX * tilesY;
			}
		} else {
			//
			// Fallback: use extracted tile count
			//
			totalGridTiles = extractedCount;
		}
		if (totalGridTiles == 0) {
			ImGui::Text("No tiles to display");
			return result;
		}
		//
		// Calculate tile display size (use a reasonable default)
		//
		const float tileDisplaySize = 64.0f;
		const float tilePadding = 8.0f;
		const float totalTileWidth = tileDisplaySize + tilePadding;
		//
		// Get available width for wrapping
		//
		ImVec2 availableRegion = ImGui::GetContentRegionAvail();
		float availableWidth = availableRegion.x;
		//
		// Calculate tiles per row and precompute extracted-index mapping for the list clipper
		//
		int tilesPerRow = std::max(1, (int)((availableWidth + tilePadding) / totalTileWidth));
		int numRows = (totalGridTiles + tilesPerRow - 1) / tilesPerRow;
		float rowHeight = tileDisplaySize + ImGui::GetStyle().FramePadding.y * 2.0f + ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
		//
		// Precompute absolute-to-extracted index mapping (deleted tiles map to -1)
		//
		std::vector<int> absToExtracted(totalGridTiles, -1);
		{
			int extIdx = 0;
			for (int ai = 0; ai < totalGridTiles; ai++) {
				bool del = tileParams && std::find(tileParams->DeletedTiles.begin(), tileParams->DeletedTiles.end(), ai) != tileParams->DeletedTiles.end();
				if (!del)
					absToExtracted[ai] = extIdx++;
			}
		}
		//
		// Render visible rows only using list clipper
		//
		ImGuiListClipper clipper;
		clipper.Begin(numRows, rowHeight);
		while (clipper.Step()) {
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
				for (int col = 0; col < tilesPerRow; col++) {
					int absoluteIndex = row * tilesPerRow + col;
					if (absoluteIndex >= totalGridTiles)
						break;
					if (col > 0)
						ImGui::SameLine(0.0f, tilePadding);
					//
					// Look up deleted state and extracted tile index from precomputed mapping
					//
					bool isDeleted = (absToExtracted[absoluteIndex] == -1);
					std::shared_ptr<RetrodevLib::Image> tileImage;
					SDL_Texture* tileTexture = nullptr;
					if (!isDeleted) {
						tileImage = tileExtractor->GetTile(absToExtracted[absoluteIndex]);
						if (tileImage)
							tileTexture = tileImage->GetTexture(Application::GetRenderer());
					}
					//
					// Push ID for this tile (use absolute index)
					//
					ImGui::PushID(absoluteIndex);
					//
					// Create a selectable frame for the tile
					//
					ImVec2 cursorPos = ImGui::GetCursorScreenPos();
					ImGui::BeginGroup();
					//
					// Draw tile image button or deleted placeholder
					//
					if (isDeleted) {
						//
						// Draw deleted tile placeholder (red box with X)
						//
						ImVec2 buttonSize = ImVec2(tileDisplaySize, tileDisplaySize);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.0f, 0.0f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.0f, 0.0f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, 1.0f));
						if (ImGui::Button("##deleted", buttonSize)) {
							result.tileSelected = true;
							result.selectedTileIndex = absoluteIndex;
						}
						ImGui::PopStyleColor(3);
						//
						// Draw red X overlay
						//
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImVec2 pMin = cursorPos;
						ImVec2 pMax = ImVec2(cursorPos.x + tileDisplaySize, cursorPos.y + tileDisplaySize);
						drawList->AddLine(pMin, pMax, IM_COL32(255, 0, 0, 255), 3.0f);
						drawList->AddLine(ImVec2(pMin.x, pMax.y), ImVec2(pMax.x, pMin.y), IM_COL32(255, 0, 0, 255), 3.0f);
					} else if (tileTexture) {
						//
						// Draw normal tile image button
						//
						if (ImGui::ImageButton("##tile", (ImTextureID)tileTexture, ImVec2(tileDisplaySize, tileDisplaySize))) {
							result.tileSelected = true;
							result.selectedTileIndex = absoluteIndex;
						}
					}
					//
					// Context menu for tile operations
					//
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
					if (ImGui::BeginPopupContextItem("TileContextMenu")) {
						ImGui::Text("Tile #%d", absoluteIndex);
						if (isDeleted) {
							ImGui::Text("(Deleted)");
						}
						ImGui::Separator();
						//
						// Show Delete or Undelete based on state
						//
						if (isDeleted) {
							if (ImGui::MenuItem("Undelete")) {
								result.tileDeleted = true;
								result.deletedTileIndex = absoluteIndex;
							}
						} else {
							if (ImGui::MenuItem("Delete")) {
								result.tileDeleted = true;
								result.deletedTileIndex = absoluteIndex;
							}
						}
						ImGui::EndPopup();
					}
					ImGui::PopStyleVar();
					//
					// Tooltip with tile info
					//
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Tile #%d", absoluteIndex);
						if (isDeleted) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DELETED");
						} else if (tileImage) {
							ImGui::Text("Size: %dx%d", tileImage->GetWidth(), tileImage->GetHeight());
						}
						ImGui::EndTooltip();
					}
					//
					// Draw tile index below the image
					//
					ImVec2 textSize = ImGui::CalcTextSize(std::to_string(absoluteIndex).c_str());
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (tileDisplaySize - textSize.x) * 0.5f);
					if (isDeleted) {
						ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%d", absoluteIndex);
					} else {
						ImGui::Text("%d", absoluteIndex);
					}
					ImGui::EndGroup();
					ImGui::PopID();
				}
			}
		}
		clipper.End();
		return result;
	}
} // namespace RetrodevGui
