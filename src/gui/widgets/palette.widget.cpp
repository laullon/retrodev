//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "palette.widget.h"
#include <app/app.console.h>

namespace RetrodevGui {

	// Static member definitions
	bool PaletteWidget::m_paletteOpen = true;
	std::vector<PaletteWidget::PenState> PaletteWidget::m_penStates;
	int PaletteWidget::m_selectedPen = -1;
	int PaletteWidget::m_selectedColorIndex = 0;
	bool PaletteWidget::m_showColorPicker = false;
	bool PaletteWidget::m_showTransparencyPicker = false;
	bool PaletteWidget::m_pickingFromImage = false;
	bool PaletteWidget::m_allLocked = false;
	bool PaletteWidget::m_allDisabled = false;

	//
	// Main render function for the palette widget, called from within an existing child window
	//
	bool PaletteWidget::Render(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette, bool showTransparency) {
		if (!params || !palette)
			return false;
		bool changed = false;
		//
		// Transparency section (compact, put first) — skipped when caller opts out
		//
		if (showTransparency)
			changed |= RenderTransparencySection(params);
		//
		// Palette section
		//
		ImGui::SetNextItemOpen(m_paletteOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Palette")) {
			m_paletteOpen = true;
			changed |= RenderPaletteColors(params, palette);
		} else {
			m_paletteOpen = false;
		}
		//
		// Color picker popup (modal)
		//
		if (m_showColorPicker) {
			changed |= RenderColorPickerPopup(params, palette);
		}
		return changed;
	}

	void PaletteWidget::EnsurePenStates(int penCount) {
		//
		// Resize pen states vector if needed
		//
		if ((int)m_penStates.size() != penCount) {
			m_penStates.resize(penCount);
			//
			// Initialize new entries
			//
			for (auto& state : m_penStates) {
				state.disabled = false;
				state.locked = false;
			}
		}
	}

	bool PaletteWidget::RenderPaletteColors(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette) {
		bool changed = false;
		//
		// Get number of pens from palette
		//
		int penCount = palette->PaletteMaxColors();
		EnsurePenStates(penCount);
		//
		// Sync pen states from palette (which points to params arrays)
		//
		for (int i = 0; i < penCount; i++) {
			m_penStates[i].locked = palette->PenGetLock(i);
			m_penStates[i].disabled = !palette->PenGetEnabled(i);
		}
		//
		// Derive "all" states from individual pen states
		//
		m_allLocked = (penCount > 0);
		m_allDisabled = (penCount > 0);
		for (int i = 0; i < penCount; i++) {
			if (!m_penStates[i].locked)
				m_allLocked = false;
			if (!m_penStates[i].disabled)
				m_allDisabled = false;
		}
		//
		// Display palette pens in a compact horizontal layout with wrapping
		//
		const float colorBoxSize = 40.0f;
		const float checkboxSize = 20.0f;
		const float allCheckboxSize = 16.0f;
		const float labelWidth = 95.0f;
		const float itemSpacing = 4.0f;
		//
		// Calculate how many pens fit per row
		//
		float availableWidth = ImGui::GetContentRegionAvail().x;
		int pensPerRow = (int)((availableWidth - labelWidth) / (colorBoxSize + itemSpacing));
		if (pensPerRow < 1)
			pensPerRow = 1;
		//
		// Push compact spacing
		//
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(itemSpacing, 2.0f));
		//
		// Render pens in rows
		//
		for (int rowStart = 0; rowStart < penCount; rowStart += pensPerRow) {
			int rowEnd = rowStart + pensPerRow;
			if (rowEnd > penCount)
				rowEnd = penCount;
			//
			// Calculate starting X position for color boxes (after label)
			//
			float startX = ImGui::GetCursorPosX() + labelWidth;
			float checkboxOffset = (colorBoxSize - checkboxSize) / 2.0f;
			//
			// Row 1: Disabled checkboxes (above color boxes)
			//
			{
				float rowY = ImGui::GetCursorPosY();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Disabled:");
				if (rowStart == 0) {
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(labelWidth - allCheckboxSize - itemSpacing, rowY + (checkboxSize - allCheckboxSize) * 0.5f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));
					if (ImGui::Checkbox("##disabledAll", &m_allDisabled)) {
						palette->PenEnableAll(!m_allDisabled);
						changed = true;
					}
					ImGui::PopStyleVar();
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(m_allDisabled ? "Enable all pens" : "Disable all pens");
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), rowY));
				}
			}
			for (int i = rowStart; i < rowEnd; i++) {
				int localIndex = i - rowStart;
				float penX = startX + localIndex * (colorBoxSize + itemSpacing);
				ImGui::SetCursorPosX(penX + checkboxOffset);
				ImGui::PushID(i * 1000 + 1);
				if (ImGui::Checkbox("##disabled", &m_penStates[i].disabled)) {
					//
					// Update palette (which will update params arrays directly)
					//
					palette->PenEnable(i, !m_penStates[i].disabled);
					changed = true;
				}
				ImGui::PopID();
				ImGui::SameLine();
			}
			ImGui::NewLine();
			//
			// Row 2: Color boxes with pen numbers
			//
			ImGui::SetCursorPosX(startX);
			for (int i = rowStart; i < rowEnd; i++) {
				if (i > rowStart)
					ImGui::SameLine();
				//
				// Color box (clickable only if locked)
				//
				RetrodevLib::RgbColor color = palette->PenGetColor(i);
				int systemIndex = palette->PenGetColorIndex(i);
				ImVec4 imColor = ImVec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, 1.0f);
				ImVec2 boxPos = ImGui::GetCursorScreenPos();
				ImGui::PushID(i * 1000);
				//
				// Only allow color selection if pen is locked
				// Unlocked pens will be reassigned during quantization
				//
				bool isLocked = m_penStates[i].locked;
				if (ImGui::ColorButton("##colorbox", imColor, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoBorder, ImVec2(colorBoxSize, colorBoxSize))) {
					if (isLocked) {
						//
						// Open color picker for locked pen
						// Initialize selected color index to current pen color
						//
						m_selectedPen = i;
						m_selectedColorIndex = systemIndex;
						m_showColorPicker = true;
					}
				}
				if (ImGui::IsItemHovered()) {
					if (isLocked) {
						ImGui::SetTooltip("Pen %d (Locked)\nSystem Color: #%d\nClick to change color", i, systemIndex);
					} else {
						ImGui::SetTooltip("Pen %d\nSystem Color: #%d\n(Lock pen to manually set color)", i, systemIndex);
					}
				}
				ImGui::PopID();
				//
				// Draw pen number centered on top of color box
				//
				char penText[8];
				snprintf(penText, sizeof(penText), "%d", i);
				ImVec2 textSize = ImGui::CalcTextSize(penText);
				ImVec2 textPos = ImVec2(boxPos.x + (colorBoxSize - textSize.x) / 2.0f, boxPos.y + (colorBoxSize - textSize.y) / 2.0f);
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				//
				// Draw text with black outline for visibility
				//
				drawList->AddText(ImVec2(textPos.x - 1, textPos.y), IM_COL32(0, 0, 0, 255), penText);
				drawList->AddText(ImVec2(textPos.x + 1, textPos.y), IM_COL32(0, 0, 0, 255), penText);
				drawList->AddText(ImVec2(textPos.x, textPos.y - 1), IM_COL32(0, 0, 0, 255), penText);
				drawList->AddText(ImVec2(textPos.x, textPos.y + 1), IM_COL32(0, 0, 0, 255), penText);
				drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), penText);
			}
			//
			// Row 3: Locked checkboxes (below color boxes)
			//
			{
				float rowY = ImGui::GetCursorPosY();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Locked:");
				if (rowStart == 0) {
					ImGui::SameLine();
					ImGui::SetCursorPos(ImVec2(labelWidth - allCheckboxSize - itemSpacing, rowY + (checkboxSize - allCheckboxSize) * 0.5f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.0f, 1.0f));
					if (ImGui::Checkbox("##lockedAll", &m_allLocked)) {
						palette->PenLockAll(m_allLocked);
						changed = true;
					}
					ImGui::PopStyleVar();
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(m_allLocked ? "Unlock all pens" : "Lock all pens");
					ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), rowY));
				}
			}
			for (int i = rowStart; i < rowEnd; i++) {
				int localIndex = i - rowStart;
				float penX = startX + localIndex * (colorBoxSize + itemSpacing);
				ImGui::SetCursorPosX(penX + checkboxOffset);
				ImGui::PushID(i * 1000 + 2);
				if (ImGui::Checkbox("##locked", &m_penStates[i].locked)) {
					//
					// Update palette (which will update params arrays directly)
					//
					palette->PenLock(i, m_penStates[i].locked);
					changed = true;
				}
				ImGui::PopID();
				ImGui::SameLine();
			}
			ImGui::NewLine();
			//
			// Add spacing between rows if not last row
			//
			if (rowEnd < penCount) {
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
			}
		}
		//
		// Pop compact spacing
		//
		ImGui::PopStyleVar();
		return changed;
	}

	bool PaletteWidget::RenderColorPickerPopup(RetrodevLib::GFXParams* params, std::shared_ptr<RetrodevLib::IPaletteConverter> palette) {
		bool changed = false;
		//
		// Open popup if not already open
		//
		if (m_showColorPicker && !ImGui::IsPopupOpen("ColorPicker")) {
			ImGui::OpenPopup("ColorPicker");
		}
		//
		// Modal popup for color selection
		//
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopupModal("ColorPicker", &m_showColorPicker, ImGuiWindowFlags_AlwaysAutoResize)) {
			if (m_selectedPen >= 0) {
				ImGui::Text("Select color for Pen %d", m_selectedPen);
				ImGui::Separator();
				//
				// Get the system palette information
				//
				int systemMaxColors = palette->GetSystemMaxColors();
				int currentIndex = palette->PenGetColorIndex(m_selectedPen);
				//
				// Display system palette as a grid of color buttons
				// Use persistent m_selectedColorIndex instead of local variable
				//
				const float swatchSize = 30.0f;
				const int colorsPerRow = 9;
				for (int i = 0; i < systemMaxColors; i++) {
					RetrodevLib::RgbColor sysColor = palette->GetSystemColorByIndex(i);
					ImVec4 imColor = ImVec4(sysColor.r / 255.0f, sysColor.g / 255.0f, sysColor.b / 255.0f, 1.0f);
					ImGui::PushID(i);
					//
					// Highlight current selection with yellow border
					//
					bool isSelected = (i == m_selectedColorIndex);
					if (isSelected) {
						ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
						ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 3.0f);
					}
					//
					// Don't use NoBorder flag - we want to see the selection border!
					//
					if (ImGui::ColorButton("##syscolor", imColor, ImGuiColorEditFlags_NoAlpha, ImVec2(swatchSize, swatchSize))) {
						//
						// Update persistent selection (survives across frames)
						//
						m_selectedColorIndex = i;
					}
					if (isSelected) {
						ImGui::PopStyleVar();
						ImGui::PopStyleColor();
					}
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("#%d\nR:%d G:%d B:%d", i, sysColor.r, sysColor.g, sysColor.b);
					}
					ImGui::PopID();
					//
					// Arrange in grid
					//
					if ((i + 1) % colorsPerRow != 0 && i < systemMaxColors - 1) {
						ImGui::SameLine();
					}
				}
				ImGui::Separator();
				//
				// Buttons
				//
				if (ImGui::Button("OK", ImVec2(120, 0))) {
					//
					// Apply the selected color index (use persistent member variable)
					//
					if (m_selectedColorIndex != currentIndex) {
						palette->PenSetColorIndex(m_selectedPen, m_selectedColorIndex);
						//
						// Persist the chosen color into params so it survives serialization
						//
						if (params) {
							if ((int)params->SParams.PaletteColors.size() <= m_selectedPen)
								params->SParams.PaletteColors.resize(m_selectedPen + 1, -1);
							params->SParams.PaletteColors[m_selectedPen] = m_selectedColorIndex;
						}
						changed = true;
					}
					m_showColorPicker = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", ImVec2(120, 0))) {
					m_showColorPicker = false;
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		return changed;
	}

	bool PaletteWidget::RenderTransparencySection(RetrodevLib::GFXParams* params) {
		bool changed = false;
		//
		// Ultra-compact transparency section - everything on one line
		//
		if (ImGui::CollapsingHeader("Transparency")) {
			//
			// Show compact warning when in picking mode
			//
			if (m_pickingFromImage) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
				ImGui::TextUnformatted("Click on image to pick color");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::SmallButton("Cancel")) {
					m_pickingFromImage = false;
				}
				ImGui::Separator();
			}
			//
			// Single line layout: Enable | Checkbox | Color box | Pick button | Slider | Label
			//
			const float itemHeight = 24.0f;
			const float colorBoxSize = 24.0f;
			const float buttonWidth = 90.0f;
			const float sliderWidth = 150.0f;
			//
			// "Enable" label
			//
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Enable");
			ImGui::SameLine();
			//
			// Checkbox (no label)
			//
			if (ImGui::Checkbox("##useTransparent", &params->RParams.UseTransparentColor)) {
				changed = true;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Enable/disable transparent color (chroma key)");
			}
			//
			// Show remaining controls if enabled (same line)
			//
			if (params->RParams.UseTransparentColor) {
				ImVec4 transparentColor =
					ImVec4(params->RParams.TransparentColorR / 255.0f, params->RParams.TransparentColorG / 255.0f, params->RParams.TransparentColorB / 255.0f, 1.0f);
				//
				// Color box (same line)
				//
				ImGui::SameLine();
				if (ImGui::ColorButton("##transparent", transparentColor, ImGuiColorEditFlags_NoAlpha, ImVec2(colorBoxSize, colorBoxSize))) {
					m_showTransparencyPicker = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("R:%d G:%d B:%d\nClick to edit", params->RParams.TransparentColorR, params->RParams.TransparentColorG, params->RParams.TransparentColorB);
				}
				//
				// Pick button (same line, compact)
				//
				ImGui::SameLine();
				if (ImGui::Button("Pick", ImVec2(buttonWidth, itemHeight))) {
					m_pickingFromImage = true;
					AppConsole::AddLog(AppConsole::LogLevel::Info, "Click on the source image to select transparent color");
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Pick color from source image");
				}
				//
				// Tolerance slider (same line)
				//
				ImGui::SameLine();
				ImGui::SetNextItemWidth(sliderWidth);
				if (ImGui::SliderInt("##tolerance", &params->RParams.TransparentColorTolerance, 0, 50)) {
					changed = true;
				}
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("Tolerance: %d\n0 = exact match\n10-30 = similar colors", params->RParams.TransparentColorTolerance);
				}
				ImGui::SameLine();
				ImGui::Text("Tol");
			}
		}
		//
		// Color picker popup for transparency color
		//
		if (m_showTransparencyPicker) {
			ImGui::OpenPopup("Transparency Color Picker");
			m_showTransparencyPicker = false;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopupModal("Transparency Color Picker", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Select transparent color:");
			ImGui::Separator();
			//
			// RGB sliders
			//
			int r = params->RParams.TransparentColorR;
			int g = params->RParams.TransparentColorG;
			int b = params->RParams.TransparentColorB;
			if (ImGui::SliderInt("Red", &r, 0, 255)) {
				params->RParams.TransparentColorR = r;
				changed = true;
			}
			if (ImGui::SliderInt("Green", &g, 0, 255)) {
				params->RParams.TransparentColorG = g;
				changed = true;
			}
			if (ImGui::SliderInt("Blue", &b, 0, 255)) {
				params->RParams.TransparentColorB = b;
				changed = true;
			}
			//
			// Preview
			//
			ImVec4 previewColor = ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Preview:");
			ImGui::SameLine();
			ImGui::ColorButton("##preview", previewColor, ImGuiColorEditFlags_NoAlpha, ImVec2(60, 60));
			ImGui::Separator();
			//
			// Common presets
			//
			ImGui::Text("Presets:");
			if (ImGui::Button("Magenta (255,0,255)", ImVec2(150, 0))) {
				params->RParams.TransparentColorR = 255;
				params->RParams.TransparentColorG = 0;
				params->RParams.TransparentColorB = 255;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Green (0,255,0)", ImVec2(150, 0))) {
				params->RParams.TransparentColorR = 0;
				params->RParams.TransparentColorG = 255;
				params->RParams.TransparentColorB = 0;
				changed = true;
			}
			if (ImGui::Button("Cyan (0,255,255)", ImVec2(150, 0))) {
				params->RParams.TransparentColorR = 0;
				params->RParams.TransparentColorG = 255;
				params->RParams.TransparentColorB = 255;
				changed = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Black (0,0,0)", ImVec2(150, 0))) {
				params->RParams.TransparentColorR = 0;
				params->RParams.TransparentColorG = 0;
				params->RParams.TransparentColorB = 0;
				changed = true;
			}
			ImGui::Separator();
			//
			// Buttons
			//
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		return changed;
	}

	void PaletteWidget::SetPickedColor(int r, int g, int b, RetrodevLib::GFXParams* params) {
		if (!params)
			return;
		//
		// Set the transparent color from picked values
		//
		params->RParams.TransparentColorR = r;
		params->RParams.TransparentColorG = g;
		params->RParams.TransparentColorB = b;
		//
		// Enable transparency if it wasn't already
		//
		params->RParams.UseTransparentColor = true;
		//
		// Done picking
		//
		m_pickingFromImage = false;
		AppConsole::AddLogF(AppConsole::LogLevel::Info, "Transparent color set to R:%d G:%d B:%d", r, g, b);
	}

} // namespace RetrodevGui
