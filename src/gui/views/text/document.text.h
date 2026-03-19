// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <imgui.text.editor.h>
#include <string>

namespace RetrodevGui {

	class DocumentText : public DocumentView {
	public:
		DocumentText(const std::string& name, const std::string& filepath);
		~DocumentText() override;
		//
		// Render the text editor document content
		//
		void Perform() override;
		//
		// Save the current editor content to disk (satisfies DocumentView::Save).
		//
		bool Save() override;
		//
		// Scroll the editor to the given line and place the cursor there.
		//
		void ScrollToLine(int line) override;
		//
		// Opens or activates a text document and schedules a scroll to the given line.
		//
		static void OpenAtLine(const std::string& filePath, int line);
		//
		// Trigger a codelens refresh on all open DocumentText instances.
		// Called when a shared setting (e.g. timing type) changes that affects all editors.
		//
		static void RefreshAllCodeLens();

	private:
		//
		// Saves the current editor content to disk and updates the modified baseline.
		//
		bool SaveDocument();
		//
		// Renders the toolbar row: status text, action buttons and their popups.
		//
		void RenderToolbar();
		//
		// Renders the bottom search/replace panel when active.
		//
		void RenderSearchPanel();
		//
		// Renders the text editor with the current font scale.
		// Returns true when the editor child window is focused.
		//
		bool RenderEditor();
		ImGui::TextEditor m_editor;
		//
		// Text captured at load time, used to detect unsaved changes
		//
		std::string m_originalText;
		//
		// UI state for toolbar controls
		//
		float m_fontScale = 1.0f;
		//
		// Bottom search/replace panel mode
		//
		enum class PanelMode { None, Find, Replace };
		PanelMode m_panelMode = PanelMode::None;
		bool m_focusSearchInput = false;
		bool m_searchCaseSensitive = true;
		char m_searchFindBuffer[256] = {};
		char m_searchReplaceBuffer[256] = {};
		//
		// Word under cursor captured at right-click time, used by the context menu.
		//
		std::string m_contextMenuWord;
		//
		// Symbol resolved from m_contextMenuWord at right-click time.
		//
		std::string m_contextMenuSymbolFile;
		int m_contextMenuSymbolLine = -1;
	};

} // namespace RetrodevGui
