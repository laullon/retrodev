// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Image palette widget -- displays and edits the hardware pen palette.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "image.palette.widget.h"

namespace RetrodevGui {

	int ImagePaletteWidget::m_selectedPen = -1;
	int ImagePaletteWidget::m_editR = 0;
	int ImagePaletteWidget::m_editG = 0;
	int ImagePaletteWidget::m_editB = 0;
	bool ImagePaletteWidget::m_showColorEditor = false;

	bool ImagePaletteWidget::Render(std::shared_ptr<RetrodevLib::Image> image) {
		bool changed = false;
		//
		// Guard: only INDEX8 images carry a palette worth showing
		//
		if (!image || !image->IsPaletized()) {
			ImGui::TextDisabled("No palette (RGBA image)");
			return false;
		}
		//
		// Ensure a valid pen is always selected -- default to index 0
		//
		if (m_selectedPen < 0)
			m_selectedPen = 0;
		//
		// Summary line: palette size and pens actually referenced by pixels
		//
		int paletteSize = image->GetPaletteSize();
		int pensUsed = image->CountPensUsed();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Palette: %d entries,", paletteSize);
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%d used", pensUsed);
		ImGui::Separator();
		//
		// Grid layout constants
		//
		const float swatchSize = 28.0f;
		const float swatchSpacing = 3.0f;
		const float totalCell = swatchSize + swatchSpacing;
		//
		// Scrollable child: only the swatch grid scrolls, the summary header stays fixed.
		// Size (0,0) fills all remaining space in the parent panel.
		//
		ImGui::BeginChild("PaletteSwatchScroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
		//
		// Fit as many swatches per row as the available width allows
		//
		float availW = ImGui::GetContentRegionAvail().x;
		int perRow = (int)(availW / totalCell);
		if (perRow < 1)
			perRow = 1;
		//
		// Render one swatch per palette entry
		//
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(swatchSpacing, swatchSpacing));
		for (int i = 0; i < paletteSize; i++) {
			RetrodevLib::RgbColor col = image->GetPaletteColor(i);
			ImVec4 imCol = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1.0f);
			//
			// Highlight the selected entry with a bright border
			//
			bool isSelected = (i == m_selectedPen);
			ImGui::PushID(i);
			if (isSelected) {
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.85f, 0.0f, 1.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.5f);
			}
			if (ImGui::ColorButton("##swatch", imCol, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoBorder, ImVec2(swatchSize, swatchSize))) {
				//
				// Single click: select as active paint color
				//
				m_selectedPen = i;
			}
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				//
				// Double click: open the color editor for this entry
				//
				m_selectedPen = i;
				m_editR = col.r;
				m_editG = col.g;
				m_editB = col.b;
				m_showColorEditor = true;
			}
			if (isSelected) {
				ImGui::PopStyleVar();
				ImGui::PopStyleColor();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Index %d\nR:%d G:%d B:%d\nClick to select  |  Double-click to edit", i, col.r, col.g, col.b);
			}
			ImGui::PopID();
			//
			// Wrap to next row
			//
			if ((i + 1) % perRow != 0 && i < paletteSize - 1)
				ImGui::SameLine();
		}
		ImGui::PopStyleVar();
		ImGui::EndChild();
		//
		// Color editor popup
		//
		if (m_showColorEditor)
			changed |= RenderColorEditorPopup(image);
		return changed;
	}

	bool ImagePaletteWidget::RenderColorEditorPopup(std::shared_ptr<RetrodevLib::Image> image) {
		bool changed = false;
		//
		// Open the popup on the frame it is first requested
		//
		if (!ImGui::IsPopupOpen("ImagePaletteColorEditor"))
			ImGui::OpenPopup("ImagePaletteColorEditor");
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		if (ImGui::BeginPopupModal("ImagePaletteColorEditor", &m_showColorEditor, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Edit palette entry %d", m_selectedPen);
			ImGui::Separator();
			//
			// Live preview swatch
			//
			ImVec4 preview = ImVec4(m_editR / 255.0f, m_editG / 255.0f, m_editB / 255.0f, 1.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Preview:");
			ImGui::SameLine();
			ImGui::ColorButton("##preview", preview, ImGuiColorEditFlags_NoAlpha, ImVec2(48.0f, 48.0f));
			ImGui::Separator();
			//
			// RGB sliders
			//
			ImGui::SetNextItemWidth(220.0f);
			ImGui::SliderInt("Red", &m_editR, 0, 255);
			ImGui::SetNextItemWidth(220.0f);
			ImGui::SliderInt("Green", &m_editG, 0, 255);
			ImGui::SetNextItemWidth(220.0f);
			ImGui::SliderInt("Blue", &m_editB, 0, 255);
			ImGui::Separator();
			//
			// Common color presets
			//
			ImGui::Text("Presets:");
			struct Preset {
				const char* label;
				int r, g, b;
			};
			static const Preset presets[] = {
				{"Black", 0, 0, 0},	 {"White", 255, 255, 255}, {"Red", 255, 0, 0},	  {"Green", 0, 255, 0},
				{"Blue", 0, 0, 255}, {"Yellow", 255, 255, 0},  {"Cyan", 0, 255, 255}, {"Magenta", 255, 0, 255},
			};
			for (int p = 0; p < (int)(sizeof(presets) / sizeof(presets[0])); p++) {
				if (p > 0 && p % 4 != 0)
					ImGui::SameLine();
				ImGui::PushID(p + 2000);
				ImVec4 pc = ImVec4(presets[p].r / 255.0f, presets[p].g / 255.0f, presets[p].b / 255.0f, 1.0f);
				if (ImGui::ColorButton(presets[p].label, pc, ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoTooltip, ImVec2(28.0f, 28.0f))) {
					m_editR = presets[p].r;
					m_editG = presets[p].g;
					m_editB = presets[p].b;
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", presets[p].label);
				ImGui::PopID();
			}
			ImGui::Separator();
			//
			// Confirm / cancel
			//
			if (ImGui::Button("OK", ImVec2(110, 0))) {
				RetrodevLib::RgbColor newColor(static_cast<uint8_t>(m_editR), static_cast<uint8_t>(m_editG), static_cast<uint8_t>(m_editB));
				image->SetPaletteColor(m_selectedPen, newColor);
				image->MarkModified();
				changed = true;
				m_showColorEditor = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(110, 0))) {
				m_showColorEditor = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
		return changed;
	}

}
