// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Tile list widget -- thumbnail grid of extracted tiles.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "tile.list.widget.h"
#include <app/app.h>
#include <algorithm>

namespace RetrodevGui {
	//
	// Static member definitions
	//
	std::vector<int> TileListWidget::m_selectedIndices;
	int TileListWidget::m_lastClickedIndex = -1;
	//
	// Render the tile list widget
	//
	TileListWidget::RenderResult TileListWidget::Render(std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor, const RetrodevLib::TileExtractionParams* tileParams,
														const std::vector<int>& selectedIndices, int primaryIndex, int imageWidth, int imageHeight) {
		RenderResult result;
		result.newSelection = selectedIndices;
		result.primaryIndex = primaryIndex;
		//
		// Sync internal selection state with the caller-supplied selection
		//
		m_selectedIndices = selectedIndices;
		if (!tileExtractor) {
			ImGui::Text("No tile extractor available");
			return result;
		}
		int extractedCount = tileExtractor->GetTileCount();
		if (extractedCount == 0) {
			ImGui::Text("No tiles extracted yet");
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click 'Extract Tiles' to start");
			return result;
		}
		//
		// Calculate total tiles in the extraction grid (including deleted ones)
		//
		int totalGridTiles = 0;
		if (tileParams && imageWidth > 0 && imageHeight > 0) {
			int availableWidth = imageWidth - tileParams->OffsetX;
			int availableHeight = imageHeight - tileParams->OffsetY;
			if (availableWidth >= tileParams->TileWidth && availableHeight >= tileParams->TileHeight) {
				int tilesX = 1 + ((availableWidth - tileParams->TileWidth) / (tileParams->TileWidth + tileParams->PaddingX));
				int tilesY = 1 + ((availableHeight - tileParams->TileHeight) / (tileParams->TileHeight + tileParams->PaddingY));
				totalGridTiles = tilesX * tilesY;
			}
		} else {
			totalGridTiles = extractedCount;
		}
		if (totalGridTiles == 0) {
			ImGui::Text("No tiles to display");
			return result;
		}
		//
		// Calculate tile display size and layout
		//
		const float tileDisplaySize = 64.0f;
		const float tilePadding = 8.0f;
		float fp = ImGui::GetStyle().FramePadding.x;
		float tileCellSize = tileDisplaySize + fp * 2.0f;
		float totalTileWidth = tileCellSize + tilePadding;
		ImVec2 availableRegion = ImGui::GetContentRegionAvail();
		float availableWidth = availableRegion.x;
		int tilesPerRow = std::max(1, (int)((availableWidth + tilePadding) / totalTileWidth));
		int numRows = (totalGridTiles + tilesPerRow - 1) / tilesPerRow;
		float rowHeight = tileCellSize + ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
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
		// Track whether context menu should open this frame (set inside loop, used outside PushID scope)
		//
		bool anyContextMenuOpen = ImGui::IsPopupOpen("##tilectx");
		bool openContextMenu = false;
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
					bool isDeleted = (absToExtracted[absoluteIndex] == -1);
					bool isSelected = std::find(m_selectedIndices.begin(), m_selectedIndices.end(), absoluteIndex) != m_selectedIndices.end();
					bool isPrimary = (absoluteIndex == primaryIndex);
					std::shared_ptr<RetrodevLib::Image> tileImage;
					SDL_Texture* tileTexture = nullptr;
					if (!isDeleted) {
						tileImage = tileExtractor->GetTile(absToExtracted[absoluteIndex]);
						if (tileImage)
							tileTexture = tileImage->GetTexture(Application::GetRenderer());
					} else {
						tileImage = tileExtractor->GetTileAll(absoluteIndex);
						if (tileImage)
							tileTexture = tileImage->GetTexture(Application::GetRenderer());
					}
					ImGui::PushID(absoluteIndex);
					ImVec2 cursorPos = ImGui::GetCursorScreenPos();
					ImGui::BeginGroup();
					//
					// Draw tile image button or deleted placeholder
					//
					bool buttonClicked = false;
					if (isDeleted) {
						if (tileTexture) {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1f, 0.0f, 0.0f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.0f, 0.0f, 1.0f));
							buttonClicked = ImGui::ImageButton("##tile", (ImTextureID)tileTexture, ImVec2(tileDisplaySize, tileDisplaySize), ImVec2(0, 0), ImVec2(1, 1),
															   ImVec4(0, 0, 0, 1), ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
							ImGui::PopStyleColor(3);
						} else {
							ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.0f, 0.0f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.0f, 0.0f, 1.0f));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.0f, 0.0f, 1.0f));
							buttonClicked = ImGui::Button("##deleted", ImVec2(tileCellSize, tileCellSize));
							ImGui::PopStyleColor(3);
						}
						//
						// Draw red border over the full button area
						//
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImVec2 pMin = cursorPos;
						ImVec2 pMax = ImVec2(cursorPos.x + tileCellSize, cursorPos.y + tileCellSize);
						drawList->AddRect(pMin, pMax, IM_COL32(220, 30, 30, 255), 0.0f, 0, 2.5f);
					} else if (tileTexture) {
						buttonClicked = ImGui::ImageButton("##tile", (ImTextureID)tileTexture, ImVec2(tileDisplaySize, tileDisplaySize));
					}
					//
					// Draw selection border overlay: gold for primary, white for other selected
					//
					if (isPrimary || isSelected) {
						ImDrawList* drawList = ImGui::GetWindowDrawList();
						ImVec2 pMin = cursorPos;
						ImVec2 pMax = ImVec2(cursorPos.x + tileCellSize, cursorPos.y + tileCellSize);
						ImU32 borderCol = isPrimary ? IM_COL32(255, 210, 60, 220) : IM_COL32(255, 255, 255, 200);
						drawList->AddRect(pMin, pMax, borderCol, 0.0f, 0, 2.5f);
					}
					//
					// Click handling: Ctrl toggles, Shift range-selects, plain click sets single selection
					//
					if (buttonClicked) {
						bool ctrlHeld = ImGui::GetIO().KeyCtrl;
						bool shiftHeld = ImGui::GetIO().KeyShift;
						if (ctrlHeld) {
							auto it = std::find(m_selectedIndices.begin(), m_selectedIndices.end(), absoluteIndex);
							if (it != m_selectedIndices.end())
								m_selectedIndices.erase(it);
							else
								m_selectedIndices.push_back(absoluteIndex);
							m_lastClickedIndex = absoluteIndex;
						} else if (shiftHeld && m_lastClickedIndex >= 0) {
							int lo = std::min(m_lastClickedIndex, absoluteIndex);
							int hi = std::max(m_lastClickedIndex, absoluteIndex);
							for (int k = lo; k <= hi; k++) {
								if (std::find(m_selectedIndices.begin(), m_selectedIndices.end(), k) == m_selectedIndices.end())
									m_selectedIndices.push_back(k);
							}
						} else {
							m_selectedIndices.clear();
							m_selectedIndices.push_back(absoluteIndex);
							m_lastClickedIndex = absoluteIndex;
						}
						result.selectionChanged = true;
						result.newSelection = m_selectedIndices;
						result.primaryIndex = absoluteIndex;
					}
					//
					// Right-click: select item if not already selected, then request popup
					//
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						if (std::find(m_selectedIndices.begin(), m_selectedIndices.end(), absoluteIndex) == m_selectedIndices.end()) {
							m_selectedIndices.clear();
							m_selectedIndices.push_back(absoluteIndex);
							m_lastClickedIndex = absoluteIndex;
							result.selectionChanged = true;
							result.newSelection = m_selectedIndices;
							result.primaryIndex = absoluteIndex;
						}
						openContextMenu = true;
					}
					//
					// Tooltip with tile info (suppressed while context menu is open)
					//
					if (ImGui::IsItemHovered() && !anyContextMenuOpen) {
						ImGui::BeginTooltip();
						ImGui::Text("Tile #%d", absoluteIndex);
						if (isDeleted) {
							ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "DELETED");
						} else if (tileImage) {
							ImGui::Text("Size: %dx%d", tileImage->GetWidth(), tileImage->GetHeight());
						}
						if (m_selectedIndices.size() > 1)
							ImGui::Text("%d tiles selected", (int)m_selectedIndices.size());
						ImGui::EndTooltip();
					}
					//
					// Draw tile index below the image, centred under the full button width
					//
					ImVec2 textSize = ImGui::CalcTextSize(std::to_string(absoluteIndex).c_str());
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (tileCellSize - textSize.x) * 0.5f);
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
		//
		// Open context menu outside all PushID scopes so the popup ID resolves correctly
		//
		if (openContextMenu)
			ImGui::OpenPopup("##tilectx");
		//
		// Context menu: rendered once, operates on the full selection
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopup("##tilectx")) {
			int selCount = static_cast<int>(m_selectedIndices.size());
			if (selCount > 1) {
				ImGui::TextDisabled("%d tiles selected", selCount);
				ImGui::Separator();
			}
			//
			// Determine if the selection is a mixed, all-deleted or all-active set
			//
			int deletedInSelection = 0;
			for (int idx : m_selectedIndices) {
				if (tileParams && std::find(tileParams->DeletedTiles.begin(), tileParams->DeletedTiles.end(), idx) != tileParams->DeletedTiles.end())
					deletedInSelection++;
			}
			bool allDeleted = (deletedInSelection == selCount);
			bool allActive = (deletedInSelection == 0);
			if (allDeleted) {
				if (ImGui::MenuItem(selCount > 1 ? "Undelete All" : "Undelete")) {
					result.tileToggled = true;
					result.toggleIndices = m_selectedIndices;
				}
			} else if (allActive) {
				if (ImGui::MenuItem(selCount > 1 ? "Delete All" : "Delete")) {
					result.tileToggled = true;
					result.toggleIndices = m_selectedIndices;
				}
			} else {
				//
				// Mixed selection: offer both options explicitly
				//
				if (ImGui::MenuItem("Delete Active")) {
					result.tileToggled = true;
					for (int idx : m_selectedIndices) {
						bool del = tileParams && std::find(tileParams->DeletedTiles.begin(), tileParams->DeletedTiles.end(), idx) != tileParams->DeletedTiles.end();
						if (!del)
							result.toggleIndices.push_back(idx);
					}
				}
				if (ImGui::MenuItem("Undelete Deleted")) {
					result.tileToggled = true;
					for (int idx : m_selectedIndices) {
						bool del = tileParams && std::find(tileParams->DeletedTiles.begin(), tileParams->DeletedTiles.end(), idx) != tileParams->DeletedTiles.end();
						if (del)
							result.toggleIndices.push_back(idx);
					}
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		return result;
	}
}
