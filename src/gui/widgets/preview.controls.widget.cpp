//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "preview.controls.widget.h"

namespace RetrodevGui {
	//
	// Render preview controls (compact single line at top of preview panel)
	//
	bool PreviewControlsWidget::Render(bool* aspectCorrection, int* scaleFactor, bool* scanlines, std::shared_ptr<RetrodevLib::IBitmapConverter> converter,
									   RetrodevLib::GFXParams* params) {
		bool changed = false;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
		//
		// Aspect correction checkbox
		//
		if (ImGui::Checkbox("Aspect", aspectCorrection))
			changed = true;
		ImGui::SameLine();
		//
		// Scanlines checkbox (disabled if aspect correction is off)
		//
		ImGui::BeginDisabled(!(*aspectCorrection));
		if (ImGui::Checkbox("Scanlines", scanlines)) {
			changed = true;
		}
		ImGui::EndDisabled();
		//
		// If aspect correction is disabled, force scanlines off
		//
		if (!(*aspectCorrection) && *scanlines) {
			*scanlines = false;
			changed = true;
		}
		ImGui::PopStyleVar();
		//
		// Apply preview settings if changed
		// Always set scale to 1 (removed from UI)
		//
		if (changed && converter) {
			*scaleFactor = 1;
			converter->SetPreviewParams(*aspectCorrection, *scaleFactor, *scanlines);
			converter->Convert(params);
		}
		return changed;
	}
	//
	// Render simplified preview controls without conversion trigger
	//
	bool PreviewControlsWidget::RenderSimple(bool* aspectCorrection, bool* scanlines) {
		bool changed = false;
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));
		//
		// Aspect correction checkbox
		//
		if (ImGui::Checkbox("Aspect", aspectCorrection))
			changed = true;
		ImGui::SameLine();
		//
		// Scanlines checkbox (disabled if aspect correction is off)
		//
		ImGui::BeginDisabled(!(*aspectCorrection));
		if (ImGui::Checkbox("Scanlines", scanlines)) {
			changed = true;
		}
		ImGui::EndDisabled();
		//
		// If aspect correction is disabled, force scanlines off
		//
		if (!(*aspectCorrection) && *scanlines) {
			*scanlines = false;
			changed = true;
		}
		ImGui::PopStyleVar();
		return changed;
	}
} // namespace RetrodevGui
