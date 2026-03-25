// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Data/hex viewer document.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "document.data.h"
#include <retrodev.gui.h>
#include <fstream>
#include <cstring>

namespace RetrodevGui {

	DocumentData::DocumentData(const std::string& name, const std::string& filepath) : DocumentView(name, filepath) {
		// Load the file into the buffer
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);
		if (file.is_open()) {
			std::streamsize size = file.tellg();
			file.seekg(0, std::ios::beg);
			if (size >= 0) {
				m_data.resize(static_cast<size_t>(size));
				if (size == 0 || file.read(reinterpret_cast<char*>(m_data.data()), size))
					m_loadOk = true;
			}
		}
		// Wire the hex editor state to the loaded buffer
		m_hexState.Bytes = m_data.empty() ? nullptr : m_data.data();
		m_hexState.MaxBytes = static_cast<int>(m_data.size());
		m_hexState.ReadOnly = true;
		m_hexState.ShowAscii = true;
		m_hexState.RenderZeroesDisabled = true;
		m_hexState.UserData = this;
		// WriteCallback intercepts every byte write so the document can track modifications
		m_hexState.WriteCallback = [](ImGuiHexEditorState* state, int offset, void* buf, int size) -> int {
			uint8_t* dst = static_cast<uint8_t*>(state->Bytes) + offset;
			std::memcpy(dst, buf, static_cast<size_t>(size));
			static_cast<DocumentData*>(state->UserData)->SetModified(true);
			return size;
		};
	}

	DocumentData::~DocumentData() {}

	//
	// Write the in-memory buffer back to disk and clear the modified flag
	//
	bool DocumentData::SaveDocument() {
		std::ofstream file(m_filePath, std::ios::binary | std::ios::trunc);
		if (!file.is_open())
			return false;
		if (!m_data.empty())
			file.write(reinterpret_cast<const char*>(m_data.data()), static_cast<std::streamsize>(m_data.size()));
		if (!file.good())
			return false;
		SetModified(false);
		return true;
	}

	bool DocumentData::Save() {
		return SaveDocument();
	}

	//
	// Render the hex viewer for the loaded data file
	//
	void DocumentData::Perform() {
		// Report load errors early
		if (!m_loadOk) {
			ImGui::TextDisabled("Failed to load: %s", m_filePath.c_str());
			return;
		}
		// Size info bar
		int byteCount = static_cast<int>(m_data.size());
		if (byteCount == 0) {
			ImGui::TextDisabled("(empty file)");
			return;
		}
		// Toolbar: read-only toggle, save button, file size
		ImGui::Checkbox("Read Only", &m_hexState.ReadOnly);
		ImGui::SameLine();
		ImGui::BeginDisabled(!IsModified());
		if (ImGui::Button("Save"))
			SaveDocument();
		ImGui::EndDisabled();
		ImGui::SameLine();
		// Format the file size in a human-readable way
		if (byteCount < 1024)
			ImGui::Text("%d bytes", byteCount);
		else if (byteCount < 1024 * 1024)
			ImGui::Text("%.1f KB  (%d bytes)", byteCount / 1024.0f, byteCount);
		else
			ImGui::Text("%.2f MB  (%d bytes)", byteCount / (1024.0f * 1024.0f), byteCount);
		// Hex editor fills the remaining available space
		ImVec2 available = ImGui::GetContentRegionAvail();
		ImGui::BeginHexEditor("##DataHex", &m_hexState, available);
		ImGui::EndHexEditor();
	}

}
