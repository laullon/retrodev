// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Sprite extraction widget -- region selector and sprite list management.
//
// (c) TLOTB 2026
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
	bool SpriteExtractionWidget::m_constrainToGrid = false;
	int SpriteExtractionWidget::m_constraintW = 1;
	int SpriteExtractionWidget::m_constraintH = 1;
	int SpriteExtractionWidget::m_prevConstraintW = 1;
	int SpriteExtractionWidget::m_prevConstraintH = 1;
	int SpriteExtractionWidget::m_prevSpriteWidth = 0;
	int SpriteExtractionWidget::m_prevSpriteHeight = 0;
	//
	// Render the sprite extraction parameters UI
	//
	SpriteExtractionWidgetResult SpriteExtractionWidget::Render(RetrodevLib::SpriteExtractionParams* spriteParams, std::shared_ptr<RetrodevLib::ISpriteExtractor> spriteExtractor,
																int selectedSpriteIndex, int constraintW, int constraintH) {
		SpriteExtractionWidgetResult result;
		if (!spriteParams)
			return result;
		//
		// Store constraint values supplied by the caller (derived from the active converter's pixel size)
		//
		int newConstraintW = (constraintW > 1) ? constraintW : 1;
		int newConstraintH = (constraintH > 1) ? constraintH : 1;
		if (newConstraintW != m_prevConstraintW || newConstraintH != m_prevConstraintH) {
			m_constrainToGrid = false;
			m_prevConstraintW = newConstraintW;
			m_prevConstraintH = newConstraintH;
		}
		m_constraintW = newConstraintW;
		m_constraintH = newConstraintH;
		//
		// Encoding alignment constraint checkbox -- disabled when the active mode imposes no constraint (1x1)
		//
		bool hasConstraint = (m_constraintW > 1 || m_constraintH > 1);
		ImGui::BeginDisabled(!hasConstraint);
		char constrainLabel[64];
		sprintf(constrainLabel, "Constrain to encoding alignment (%dx%d)", m_constraintW, m_constraintH);
		ImGui::Checkbox(constrainLabel, &m_constrainToGrid);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("When enabled, sprite width and height are snapped to multiples\nof the platform's encoding unit (%d x %d pixels).\nThis ensures each sprite "
							  "dimension fits whole bytes\nin the target system's native pixel format.",
							  m_constraintW, m_constraintH);
		}
		if (!hasConstraint)
			m_constrainToGrid = false;
		ImGui::Separator();
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
				result.cancelRequested = true;
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
			bool widthInvalid = m_constrainToGrid && m_constraintW > 1 && (spriteDef.Width % m_constraintW) != 0;
			bool heightInvalid = m_constrainToGrid && m_constraintH > 1 && (spriteDef.Height % m_constraintH) != 0;
			if (widthInvalid)
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			if (ImGui::InputInt("Width", &spriteDef.Width)) {
				if (spriteDef.Width < 1)
					spriteDef.Width = 1;
				if (m_constrainToGrid && m_constraintW > 1) {
					int cW = m_constraintW;
					if (spriteDef.Width < m_prevSpriteWidth)
						spriteDef.Width = std::max(cW, (spriteDef.Width / cW) * cW);
					else
						spriteDef.Width = std::max(cW, ((spriteDef.Width + cW - 1) / cW) * cW);
				}
				m_prevSpriteWidth = spriteDef.Width;
				changed = true;
			}
			if (widthInvalid) {
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Width %d is not a multiple of %d (encoding alignment).\nPress +/- to snap to a valid value.", spriteDef.Width, m_constraintW);
			}
			if (heightInvalid)
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			if (ImGui::InputInt("Height", &spriteDef.Height)) {
				if (spriteDef.Height < 1)
					spriteDef.Height = 1;
				if (m_constrainToGrid && m_constraintH > 1) {
					int cH = m_constraintH;
					if (spriteDef.Height < m_prevSpriteHeight)
						spriteDef.Height = std::max(cH, (spriteDef.Height / cH) * cH);
					else
						spriteDef.Height = std::max(cH, ((spriteDef.Height + cH - 1) / cH) * cH);
				}
				m_prevSpriteHeight = spriteDef.Height;
				changed = true;
			}
			if (heightInvalid) {
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Height %d is not a multiple of %d (encoding alignment).\nPress +/- to snap to a valid value.", spriteDef.Height, m_constraintH);
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
	//
	// Returns true when the pixel-grid constraint checkbox is checked
	//
	bool SpriteExtractionWidget::IsConstrainActive() {
		return m_constrainToGrid;
	}
	//
	// Returns the active horizontal size constraint multiple
	//
	int SpriteExtractionWidget::GetConstraintW() {
		return m_constraintW;
	}
	//
	// Returns the active vertical size constraint multiple
	//
	int SpriteExtractionWidget::GetConstraintH() {
		return m_constraintH;
	}
}
