// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Generic confirmation dialog.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "dialog.confirm.h"
#include <app/app.icons.mdi.h>

namespace RetrodevGui {

	void ConfirmDialog::Show(const std::string& message, std::function<void()> onConfirm) {
		m_message = message;
		m_onConfirm = std::move(onConfirm);
	}

	void ConfirmDialog::Render() {
		if (!m_message.empty())
			ImGui::OpenPopup("Confirm");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			ImGui::Text(ICON_ALERT_OUTLINE " %s", m_message.c_str());
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 172.0f) * 0.5f);
			if (ImGui::Button("Confirm", ImVec2(80.0f, 0.0f))) {
				if (m_onConfirm)
					m_onConfirm();
				m_message.clear();
				m_onConfirm = nullptr;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(80.0f, 0.0f))) {
				m_message.clear();
				m_onConfirm = nullptr;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			ImGui::EndPopup();
		}
	}

}
