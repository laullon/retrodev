// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Sprite list widget -- thumbnail grid of extracted sprites.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "sprite.list.widget.h"
#include <app/app.h>
#include <algorithm>

namespace RetrodevGui {
	//
	// Static member definitions
	//
	std::vector<int> SpriteListWidget::m_selectedIndices;
	int SpriteListWidget::m_lastClickedIndex = -1;
	//
	// Render the sprite list UI
	//
	SpriteListWidgetResult SpriteListWidget::Render(std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor, RetrodevLib::SpriteExtractionParams* spriteParams,
													const std::vector<int>& selectedIndices, int primaryIndex) {
		SpriteListWidgetResult result;
		result.newSelection = selectedIndices;
		result.primaryIndex = primaryIndex;
		//
		// Sync internal selection state with the caller-supplied selection
		//
		m_selectedIndices = selectedIndices;
		if (!spriteExtractor || !spriteParams) {
			ImGui::TextDisabled("No sprite extractor available");
			return result;
		}
		int spriteCount = spriteExtractor->GetSpriteCount();
		if (spriteCount == 0) {
			ImGui::TextDisabled("No sprites extracted yet");
			ImGui::TextWrapped("Click 'Add Sprite' to define sprite regions in the image above.");
			return result;
		}
		//
		// Calculate sprite thumbnail size and spacing
		//
		float thumbnailSize = 64.0f;
		float borderThickness = 2.0f;
		float itemSize = thumbnailSize + 2.0f * borderThickness;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float availWidth = ImGui::GetContentRegionAvail().x;
		//
		// Calculate sprites per row for list clipper
		//
		int spritesPerRow = std::max(1, (int)((availWidth + spacing) / (itemSize + spacing)));
		int numRows = (spriteCount + spritesPerRow - 1) / spritesPerRow;
		float itemHeight = itemSize + ImGui::GetStyle().ItemSpacing.y;
		//
		// Track whether a context menu is open this frame (suppresses tooltips)
		//
		bool anyContextMenuOpen = ImGui::IsPopupOpen("##spritectx");
		bool openContextMenu = false;
		//
		// Render visible rows only using list clipper
		//
		ImGuiListClipper clipper;
		clipper.Begin(numRows, itemHeight);
		while (clipper.Step()) {
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
				for (int col = 0; col < spritesPerRow; col++) {
					int i = row * spritesPerRow + col;
					if (i >= spriteCount)
						break;
					if (col > 0)
						ImGui::SameLine(0.0f, spacing);
					//
					// Get sprite image and texture
					//
					auto spriteImage = spriteExtractor->GetSprite(i);
					SDL_Texture* texture = spriteImage ? spriteImage->GetTexture(Application::GetRenderer()) : nullptr;
					auto spriteDef = spriteExtractor->GetSpriteDefinition(i);
					//
					// Use InvisibleButton for consistent row height, draw background and image via draw list
					//
					ImGui::PushID(i);
					ImVec2 bbMin = ImGui::GetCursorScreenPos();
					ImVec2 bbMax = ImVec2(bbMin.x + itemSize, bbMin.y + itemSize);
					bool isSelected = std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i) != m_selectedIndices.end();
					bool isPrimary = (i == primaryIndex);
					bool clicked = ImGui::InvisibleButton("##sprite", ImVec2(itemSize, itemSize));
					ImDrawList* dl = ImGui::GetWindowDrawList();
					//
					// Push item clip rect to prevent window clip from cutting thumbnail edges
					//
					dl->PushClipRect(bbMin, bbMax, false);
					//
					// Draw button background based on interaction state
					//
					if (isSelected)
						dl->AddRectFilled(bbMin, bbMax, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive)));
					else if (ImGui::IsItemHovered())
						dl->AddRectFilled(bbMin, bbMax, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered)));
					else
						dl->AddRectFilled(bbMin, bbMax, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Button)));
					//
					// Draw sprite image centered within the thumbnail area
					//
					if (texture && spriteImage) {
						float spriteW = static_cast<float>(spriteImage->GetWidth());
						float spriteH = static_cast<float>(spriteImage->GetHeight());
						float aspectRatio = spriteW / spriteH;
						ImVec2 imageSize;
						if (aspectRatio > 1.0f) {
							imageSize = ImVec2(thumbnailSize, thumbnailSize / aspectRatio);
						} else {
							imageSize = ImVec2(thumbnailSize * aspectRatio, thumbnailSize);
						}
						float ox = borderThickness + (thumbnailSize - imageSize.x) * 0.5f;
						float oy = borderThickness + (thumbnailSize - imageSize.y) * 0.5f;
						ImVec2 imgMin = ImVec2(bbMin.x + ox, bbMin.y + oy);
						ImVec2 imgMax = ImVec2(imgMin.x + imageSize.x, imgMin.y + imageSize.y);
						dl->AddImage((ImTextureID)(intptr_t)texture, imgMin, imgMax);
					}
					//
					// Draw border: gold for primary, white for other selected, faint white on hover
					//
					if (isPrimary) {
						float half = borderThickness * 0.5f + 0.5f;
						dl->AddRect(ImVec2(bbMin.x + half, bbMin.y + half), ImVec2(bbMax.x - half, bbMax.y - half), IM_COL32(255, 210, 60, 220), 0.0f, 0, borderThickness);
					} else if (isSelected) {
						float half = borderThickness * 0.5f + 0.5f;
						dl->AddRect(ImVec2(bbMin.x + half, bbMin.y + half), ImVec2(bbMax.x - half, bbMax.y - half), IM_COL32(255, 255, 255, 200), 0.0f, 0, borderThickness);
					} else if (ImGui::IsItemHovered()) {
						float half = borderThickness * 0.5f + 0.5f;
						dl->AddRect(ImVec2(bbMin.x + half, bbMin.y + half), ImVec2(bbMax.x - half, bbMax.y - half), IM_COL32(255, 255, 255, 120), 0.0f, 0, borderThickness);
					}
					dl->PopClipRect();
					//
					// Click handling: Ctrl toggles, Shift range-selects, plain click sets single selection
					//
					if (clicked) {
						bool ctrlHeld = ImGui::GetIO().KeyCtrl;
						bool shiftHeld = ImGui::GetIO().KeyShift;
						if (ctrlHeld) {
							//
							// Toggle this item in the selection
							//
							auto it = std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i);
							if (it != m_selectedIndices.end())
								m_selectedIndices.erase(it);
							else
								m_selectedIndices.push_back(i);
							m_lastClickedIndex = i;
						} else if (shiftHeld && m_lastClickedIndex >= 0) {
							//
							// Range select from last clicked to i (inclusive), preserve existing selection
							//
							int lo = std::min(m_lastClickedIndex, i);
							int hi = std::max(m_lastClickedIndex, i);
							for (int k = lo; k <= hi; k++) {
								if (std::find(m_selectedIndices.begin(), m_selectedIndices.end(), k) == m_selectedIndices.end())
									m_selectedIndices.push_back(k);
							}
						} else {
							//
							// Plain click: select only this item
							//
							m_selectedIndices.clear();
							m_selectedIndices.push_back(i);
							m_lastClickedIndex = i;
						}
						result.selectionChanged = true;
						result.newSelection = m_selectedIndices;
						result.primaryIndex = i;
					}
					//
					// Right-click: if item is not selected, select it first, then request the shared popup
					//
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						if (std::find(m_selectedIndices.begin(), m_selectedIndices.end(), i) == m_selectedIndices.end()) {
							m_selectedIndices.clear();
							m_selectedIndices.push_back(i);
							m_lastClickedIndex = i;
							result.selectionChanged = true;
							result.newSelection = m_selectedIndices;
							result.primaryIndex = i;
						}
						openContextMenu = true;
					}
					//
					// Show tooltip with sprite info (suppressed while context menu is open)
					//
					if (ImGui::IsItemHovered() && !anyContextMenuOpen) {
						ImGui::BeginTooltip();
						ImGui::Text("Sprite #%d", i);
						if (spriteDef) {
							if (!spriteDef->Name.empty())
								ImGui::Text("Name: %s", spriteDef->Name.c_str());
							ImGui::Text("Position: (%d, %d)", spriteDef->X, spriteDef->Y);
							ImGui::Text("Size: %dx%d", spriteDef->Width, spriteDef->Height);
						}
						if (m_selectedIndices.size() > 1)
							ImGui::Text("%d sprites selected", (int)m_selectedIndices.size());
						ImGui::EndTooltip();
					}
					ImGui::PopID();
				}
			}
		}
		clipper.End();
		//
		// Open the context menu here, outside all PushID scopes, so the popup ID resolves correctly
		//
		if (openContextMenu)
			ImGui::OpenPopup("##spritectx");
		//
		// Context menu: rendered once outside the per-item loop, operates on the full selection
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopup("##spritectx")) {
			result.operationSourceIndices = m_selectedIndices;
			int selCount = static_cast<int>(m_selectedIndices.size());
			//
			// Show header when multiple sprites are selected
			//
			if (selCount > 1) {
				ImGui::TextDisabled("%d sprites selected", selCount);
				ImGui::Separator();
			}
			if (ImGui::BeginMenu("Duplicate")) {
				if (ImGui::MenuItem("Duplicate"))
					result.duplicate = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Duplicate + Flip Horizontal"))
					result.duplicateFlipH = true;
				if (ImGui::MenuItem("Duplicate + Flip Vertical"))
					result.duplicateFlipV = true;
				ImGui::Separator();
				if (ImGui::MenuItem("Duplicate + Shift Left"))
					result.duplicateShiftLeft = true;
				if (ImGui::MenuItem("Duplicate + Shift Right"))
					result.duplicateShiftRight = true;
				if (ImGui::MenuItem("Duplicate + Shift Up"))
					result.duplicateShiftUp = true;
				if (ImGui::MenuItem("Duplicate + Shift Down"))
					result.duplicateShiftDown = true;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Flip")) {
				if (ImGui::MenuItem("Flip Horizontal"))
					result.flipH = true;
				if (ImGui::MenuItem("Flip Vertical"))
					result.flipV = true;
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Shift")) {
				if (ImGui::MenuItem("Shift Left"))
					result.shiftLeft = true;
				if (ImGui::MenuItem("Shift Right"))
					result.shiftRight = true;
				if (ImGui::MenuItem("Shift Up"))
					result.shiftUp = true;
				if (ImGui::MenuItem("Shift Down"))
					result.shiftDown = true;
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Remove"))
				result.removeSprites = true;
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		return result;
	}
}
