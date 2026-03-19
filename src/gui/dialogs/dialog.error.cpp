// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include <app/app.icons.mdi.h>
#include "dialog.error.h"

namespace RetrodevGui {

	void ErrorDialog::Show(const std::string& message) {
		m_message = message;
	}

	void ErrorDialog::Render() {
		if (!m_message.empty()) {
			ImGui::OpenPopup("Error");
		}
		if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			// Add some padding at the top
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			// Error icon and message
			ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255)); // Red text for error
			ImGui::Text(ICON_EXCLAMATION " Error");
			ImGui::PopStyleColor();
			ImGui::Spacing();

			ImGui::TextUnformatted(m_message.c_str());
			ImGui::Spacing();

			// Center the button
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 120.0f) * 0.5f);

			// Red OK button
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(200, 50, 50, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 100, 100, 255));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(150, 25, 25, 255));
			if (ImGui::Button("OK", ImVec2(120, 0))) {
				m_message.clear();
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopStyleColor(3);

			// Add some padding at the bottom
			ImGui::Dummy(ImVec2(0.0f, 10.0f));

			ImGui::EndPopup();
		}
	}
} // namespace RetrodevGui