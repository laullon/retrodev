// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "sprite.extraction.widget.h"
#include <app/app.icons.mdi.h>
#include <algorithm>
#include <string>

namespace RetrodevGui {
	//
	// Static member definitions
	//
	bool SpriteExtractionWidget::m_selectionModeActive = false;
	int SpriteExtractionWidget::m_selectionX = 0;
	int SpriteExtractionWidget::m_selectionY = 0;
	int SpriteExtractionWidget::m_selectionWidth = 16;
	int SpriteExtractionWidget::m_selectionHeight = 16;
	std::string SpriteExtractionWidget::m_pendingSpriteName = "";
	//
	// Render the sprite extraction parameters UI
	//
	SpriteExtractionWidgetResult SpriteExtractionWidget::Render(RetrodevLib::SpriteExtractionParams* spriteParams, std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor,
																int selectedSpriteIndex) {
		SpriteExtractionWidgetResult result;
		if (!spriteParams)
			return result;
		//
		// Add Sprite / Selection Mode UI
		//
		if (m_selectionModeActive) {
			//
			// Show selection mode UI with Done/Cancel buttons
			//
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Selection Mode Active");
			ImGui::Separator();
			//
			// Show current selection coordinates (live during dragging)
			//
			ImGui::Text("Selection Area:");
			ImGui::Text("X: %d", m_selectionX);
			ImGui::Text("Y: %d", m_selectionY);
			ImGui::Text("Width: %d", m_selectionWidth);
			ImGui::Text("Height: %d", m_selectionHeight);
			ImGui::Separator();
			//
			// Editable name field
			//
			ImGui::Text("Sprite Name:");
			char nameBuf[256] = {0};
			if (m_pendingSpriteName.size() < sizeof(nameBuf)) {
				memcpy(nameBuf, m_pendingSpriteName.c_str(), m_pendingSpriteName.size());
			}
			if (ImGui::InputText("##spritename", nameBuf, sizeof(nameBuf))) {
				m_pendingSpriteName = nameBuf;
			}
			ImGui::Separator();
			//
			// Done button (confirms and adds sprite)
			//
			bool validSelection = (m_selectionWidth >= 1 && m_selectionHeight >= 1);
			ImGui::BeginDisabled(!validSelection);
			if (ImGui::Button(ICON_CHECK " Done", ImVec2(-1, 0))) {
				result.addSpriteRequested = false;
				result.selectionModeActive = false;
				m_selectionModeActive = false;
			}
			ImGui::EndDisabled();
			//
			// Cancel button
			//
			if (ImGui::Button(ICON_CLOSE " Cancel", ImVec2(-1, 0))) {
				m_selectionModeActive = false;
				result.selectionModeActive = false;
			}
		} else {
			//
			// Normal mode - show Add Sprite button
			//
			if (ImGui::Button(ICON_PLUS " Add Sprite", ImVec2(-1, 0))) {
				m_selectionModeActive = true;
				m_selectionX = 0;
				m_selectionY = 0;
				m_selectionWidth = 16;
				m_selectionHeight = 16;
				//
				// Auto-generate default name based on current sprite count
				//
				m_pendingSpriteName = "Sprite" + std::to_string(spriteParams->Sprites.size());
				result.addSpriteRequested = true;
			}
		}
		ImGui::Separator();
		//
		// Show sprite count
		//
		int spriteCount = static_cast<int>(spriteParams->Sprites.size());
		ImGui::Text("Sprites: %d total", spriteCount);
		//
		// Show selected sprite details
		//
		if (selectedSpriteIndex >= 0 && selectedSpriteIndex < spriteCount) {
			ImGui::Separator();
			ImGui::Text("Selected Sprite #%d", selectedSpriteIndex);
			auto& spriteDef = spriteParams->Sprites[selectedSpriteIndex];
			//
			// Editable fields for sprite position and size
			//
			bool changed = false;
			if (ImGui::InputInt("X Position", &spriteDef.X)) {
				if (spriteDef.X < 0)
					spriteDef.X = 0;
				changed = true;
			}
			if (ImGui::InputInt("Y Position", &spriteDef.Y)) {
				if (spriteDef.Y < 0)
					spriteDef.Y = 0;
				changed = true;
			}
			if (ImGui::InputInt("Width", &spriteDef.Width)) {
				if (spriteDef.Width < 1)
					spriteDef.Width = 1;
				changed = true;
			}
			if (ImGui::InputInt("Height", &spriteDef.Height)) {
				if (spriteDef.Height < 1)
					spriteDef.Height = 1;
				changed = true;
			}
			//
			// Name input
			//
			char nameBuf[256] = {0};
			if (spriteDef.Name.size() < sizeof(nameBuf)) {
				memcpy(nameBuf, spriteDef.Name.c_str(), spriteDef.Name.size());
			}
			if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
				spriteDef.Name = nameBuf;
				changed = true;
			}
			if (changed) {
				result.parametersChanged = true;
				result.extractionTriggered = true;
			}
			//
			// Delete sprite button - actually removes from array
			//
			if (ImGui::Button(ICON_DELETE " Delete Sprite", ImVec2(-1, 0))) {
				spriteParams->Sprites.erase(spriteParams->Sprites.begin() + selectedSpriteIndex);
				result.parametersChanged = true;
				result.extractionTriggered = true;
			}
		}
		result.selectionModeActive = m_selectionModeActive;
		return result;
	}
	//
	// Check if selection mode is active
	//
	bool SpriteExtractionWidget::IsSelectionModeActive() {
		return m_selectionModeActive;
	}
	//
	// Set selection mode state
	//
	void SpriteExtractionWidget::SetSelectionMode(bool active) {
		m_selectionModeActive = active;
	}
	//
	// Get the current selection rectangle
	//
	void SpriteExtractionWidget::GetSelection(int& x, int& y, int& width, int& height) {
		x = m_selectionX;
		y = m_selectionY;
		width = m_selectionWidth;
		height = m_selectionHeight;
	}
	//
	// Set the selection rectangle (used during mouse dragging)
	//
	void SpriteExtractionWidget::SetSelection(int x, int y, int width, int height) {
		m_selectionX = x;
		m_selectionY = y;
		m_selectionWidth = width;
		m_selectionHeight = height;
	}
	//
	// Confirm the current selection (adds sprite to params)
	//
	void SpriteExtractionWidget::ConfirmSelection() {
		m_selectionModeActive = false;
	}
	//
	// Get the pending sprite name
	//
	std::string SpriteExtractionWidget::GetPendingSpriteName() {
		return m_pendingSpriteName;
	}
} // namespace RetrodevGui
