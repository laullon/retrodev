// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "sprite.list.widget.h"
#include <app/app.h>
#include <algorithm>

namespace RetrodevGui {
	//
	// Render the sprite list UI
	//
	SpriteListWidgetResult SpriteListWidget::Render(std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor, RetrodevLib::SpriteExtractionParams* spriteParams,
													int currentSelectedIndex) {
		SpriteListWidgetResult result;
		result.selectedSpriteIndex = currentSelectedIndex;
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
					bool selected = (i == currentSelectedIndex);
					bool clicked = ImGui::InvisibleButton("##sprite", ImVec2(itemSize, itemSize));
					ImDrawList* dl = ImGui::GetWindowDrawList();
					//
					// Push item clip rect to prevent window clip from cutting thumbnail edges
					//
					dl->PushClipRect(bbMin, bbMax, false);
					//
					// Draw button background based on interaction state
					//
					if (selected)
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
					// Draw selection or hover border within the allocated border zone
					//
					if (selected || ImGui::IsItemHovered()) {
						ImU32 borderColor = selected ? IM_COL32(255, 255, 255, 200) : IM_COL32(255, 255, 255, 120);
						float half = borderThickness * 0.5f + 0.5f;
						dl->AddRect(ImVec2(bbMin.x + half, bbMin.y + half), ImVec2(bbMax.x - half, bbMax.y - half), borderColor, 0.0f, 0, borderThickness);
					}
					dl->PopClipRect();
					if (clicked) {
						result.spriteSelected = true;
						result.selectedSpriteIndex = i;
					}
					//
					// Context menu on right-click: remove sprite
					//
					ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
					if (ImGui::BeginPopupContextItem("##spritectx")) {
						if (ImGui::MenuItem("Remove")) {
							result.spriteDeleted      = true;
							result.deletedSpriteIndex = i;
						}
						ImGui::EndPopup();
					}
					ImGui::PopStyleVar();
					//
					// Show tooltip with sprite info
					//
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::Text("Sprite #%d", i);
						if (spriteDef) {
							if (!spriteDef->Name.empty()) {
								ImGui::Text("Name: %s", spriteDef->Name.c_str());
							}
							ImGui::Text("Position: (%d, %d)", spriteDef->X, spriteDef->Y);
							ImGui::Text("Size: %dx%d", spriteDef->Width, spriteDef->Height);
						}
						ImGui::EndTooltip();
					}
					ImGui::PopID();
				}
			}
		}
		clipper.End();
		return result;
	}
} // namespace RetrodevGui
