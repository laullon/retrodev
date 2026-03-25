// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Data/hex viewer document.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <imgui.hex.editor.h>
#include <string>
#include <vector>
#include <cstdint>

namespace RetrodevGui {

	class DocumentData : public DocumentView {
	public:
		DocumentData(const std::string& name, const std::string& filepath);
		~DocumentData() override;
		//
		// Render the data document content (hex viewer)
		//
		void Perform() override;
		//
		// Serialize modifications to disk (satisfies DocumentView::Save)
		//
		bool Save() override;

	private:
		//
		// Write the buffer to disk and clear the modified flag
		//
		bool SaveDocument();
		//
		// Raw file contents loaded at construction time
		//
		std::vector<uint8_t> m_data;
		//
		// True when the file was read successfully (false on I/O error)
		//
		bool m_loadOk = false;
		//
		// Persistent hex editor state (selection, scroll position, etc.)
		//
		ImGuiHexEditorState m_hexState;
	};

}
