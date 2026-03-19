// --------------------------------------------------------------------------------------------------------------
//
//
// TODO: Fix change opened document on selection. Filepath mess on documents
//
// --------------------------------------------------------------------------------------------------------------

#include "main.view.documents.h"
#include <views/bitmaps/document.bitmap.h>
#include <dialogs/dialog.confirm.h>
#include <filesystem>

namespace RetrodevGui {

	//
	// Compute the forward-slash project-relative path for a given absolute file path.
	// Returns an empty string if no project is open or the path is not under the project folder.
	//
	std::string DocumentsView::ComputeProjectRelativePath(const std::string& absFilePath) {
		if (absFilePath.empty())
			return {};
		std::string projectFolder = RetrodevLib::Project::GetProjectFolder();
		if (projectFolder.empty())
			return {};
		std::error_code ec;
		std::filesystem::path rel = std::filesystem::relative(std::filesystem::path(absFilePath), std::filesystem::path(projectFolder), ec);
		if (ec)
			return {};
		return rel.generic_string();
	}

	//
	// Open a document tab
	//
	void DocumentsView::OpenDocument(std::shared_ptr<DocumentView> document) {
		// Check if the document is already open
		for (auto& doc : documents) {
			if (doc->GetName() == document->GetName()) {
				return;
			}
		}
		//
		// Compute and cache the project-relative path for log navigation matching
		//
		if (document->GetProjectRelativePath().empty())
			document->SetProjectRelativePath(ComputeProjectRelativePath(document->GetFilePath()));
		documents.push_back(std::move(document));
	}

	//
	// Close a document tab by index
	//
	void DocumentsView::CloseDocument(int index) {
		if (index >= 0 && index < (int)documents.size()) {
			documents.erase(documents.begin() + index);
			// Adjust history indices to remain valid after the removal
			if (previousDocumentIndex == index)
				previousDocumentIndex = -1;
			else if (previousDocumentIndex > index)
				previousDocumentIndex--;
			if (lastRenderedDocumentIndex == index)
				lastRenderedDocumentIndex = -1;
			else if (lastRenderedDocumentIndex > index)
				lastRenderedDocumentIndex--;
		}
	}

	//
	// Find and activate a document by name and file path (returns true if found and activated)
	//
	bool DocumentsView::ActivateDocument(const std::string& name, const std::string& filePath) {
		for (size_t i = 0; i < documents.size(); i++) {
			if (documents[i]->GetName() == name && documents[i]->GetFilePath() == filePath) {
				activeDocumentIndex = static_cast<int>(i);
				return true;
			}
		}
		return false;
	}
	//
	// Find, activate and scroll a document to the given line (returns true if found and activated).
	// Matches by project-relative path first (forward-slash form from log tokens),
	// then falls back to absolute path comparison.
	//
	bool DocumentsView::ActivateDocumentAtLine(const std::string& name, const std::string& filePath, int line) {
		//
		// Compute the project-relative form of the incoming path for canonical matching
		//
		std::string incomingRel = ComputeProjectRelativePath(filePath);
		for (size_t i = 0; i < documents.size(); i++) {
			const std::string& docRel = documents[i]->GetProjectRelativePath();
			bool match = (!incomingRel.empty() && !docRel.empty() && docRel == incomingRel) || (documents[i]->GetName() == name && documents[i]->GetFilePath() == filePath);
			if (match) {
				activeDocumentIndex = static_cast<int>(i);
				documents[i]->ScrollToLine(line);
				return true;
			}
		}
		return false;
	}
	//
	// Find and activate a document by name, file path, and build type (returns true if found and activated)
	// This is needed when multiple build items (bitmap, tileset, sprite) share the same source file
	//
	bool DocumentsView::ActivateDocument(const std::string& name, const std::string& filePath, RetrodevLib::ProjectBuildType buildType) {
		for (size_t i = 0; i < documents.size(); i++) {
			if (documents[i]->GetName() == name && documents[i]->GetFilePath() == filePath && documents[i]->GetBuildType() == buildType) {
				activeDocumentIndex = static_cast<int>(i);
				return true;
			}
		}
		return false;
	}
	//
	// Check if a document with the given name exists (returns true if found)
	//
	bool DocumentsView::IsDocumentNameOpen(const std::string& name) {
		for (const auto& doc : documents) {
			if (doc->GetName() == name)
				return true;
		}
		return false;
	}

	//
	// Clear the modified flag for all open documents
	// Called when the project is saved
	//
	void DocumentsView::ClearAllModifiedFlags() {
		for (auto& doc : documents) {
			doc->SetModified(false);
		}
	}
	//
	// Save all open documents that have unsaved modifications
	//
	void DocumentsView::SaveAllModified() {
		for (auto& doc : documents) {
			if (doc->IsModified())
				doc->Save();
		}
	}
	//
	// Notify all open documents that a project build item has changed
	//
	void DocumentsView::NotifyProjectItemChanged(const std::string& itemName) {
		for (auto& doc : documents)
			if (doc.get() != s_performingDocument)
				doc->OnProjectItemChanged(itemName);
	}
	//
	// Update the name of an open build item document after it has been renamed in the project
	//
	void DocumentsView::RenameBuildItemDocument(const std::string& oldName, const std::string& newName, RetrodevLib::ProjectBuildType buildType) {
		for (auto& doc : documents) {
			if (doc->GetName() == oldName && doc->GetBuildType() == buildType) {
				doc->SetName(newName);
				return;
			}
		}
	}
	//
	// Close an open build item document by name and build type
	//
	void DocumentsView::CloseBuildItemDocument(const std::string& name, RetrodevLib::ProjectBuildType buildType) {
		for (int i = 0; i < (int)documents.size(); i++) {
			if (documents[i]->GetName() == name && documents[i]->GetBuildType() == buildType) {
				CloseDocument(i);
				return;
			}
		}
	}
	//
	// Close all open documents and reset the active document index
	// Called when the project is closed
	//
	void DocumentsView::CloseAllDocuments() {
		documents.clear();
		activeDocumentIndex = -1;
		previousDocumentIndex = -1;
		lastRenderedDocumentIndex = -1;
	}

	//
	// Render the document tabs
	//
	void DocumentsView::Perform() {
		ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_FittingPolicyScroll;

		if (documents.empty()) {
			activeDocumentIndex = -1;
			previousDocumentIndex = -1;
			lastRenderedDocumentIndex = -1;
			return;
		}

		// Ctrl+Tab: jump to the previously active document
		ImGuiIO& io = ImGui::GetIO();
		bool isShortcut = (io.ConfigMacOSXBehaviors ? (io.KeySuper && !io.KeyCtrl) : (io.KeyCtrl && !io.KeySuper)) && !io.KeyAlt && !io.KeyShift;
		if (isShortcut && ImGui::IsKeyPressed(ImGuiKey_Tab) && previousDocumentIndex >= 0 && previousDocumentIndex < (int)documents.size())
			activeDocumentIndex = previousDocumentIndex;

		if (ImGui::BeginTabBar("DocumentTabs", tabBarFlags)) {
			int renderedThisFrame = -1;
			bool anyTabClosed = false;
			for (int i = 0; i < (int)documents.size(); i++) {
				auto& doc = documents[i];

				// Show unsaved indicator
				ImGuiTabItemFlags tabFlags = ImGuiTabItemFlags_None;
				if (doc->IsModified()) {
					tabFlags |= ImGuiTabItemFlags_UnsavedDocument;
				}
				// Set the tab as selected if it matches activeDocumentIndex
				if (i == activeDocumentIndex) {
					tabFlags |= ImGuiTabItemFlags_SetSelected;
					activeDocumentIndex = -1;
				}

				bool tabOpen = true;
				if (ImGui::BeginTabItem(doc->GetName().c_str(), &tabOpen, tabFlags)) {
					renderedThisFrame = i;
					if (ImGui::BeginChild("##DocContent", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
						s_performingDocument = doc.get();
						doc->Perform();
						s_performingDocument = nullptr;
					}
					ImGui::EndChild();
					ImGui::EndTabItem();
				}

				// Handle tab close
				if (!tabOpen) {
					if (doc->IsModified()) {
						//
						// Document has unsaved changes — ask for confirmation before closing.
						// Capture the document name as a stable identity; look it up by name
						// inside the callback so the index remains valid even if earlier tabs
						// were closed in the same session.
						//
						std::string docName = doc->GetName();
						s_pendingCloseDocumentName = docName;
						ConfirmDialog::Show(
							"\"" + docName + "\" has unsaved changes.\nClose without saving?",
							[docName]() {
								for (int j = 0; j < (int)documents.size(); j++) {
									if (documents[j]->GetName() == docName) {
										CloseDocument(j);
										break;
									}
								}
								s_pendingCloseDocumentName.clear();
							});
					} else {
						CloseDocument(i);
						anyTabClosed = true;
						i--;
					}
				}
			}
			ImGui::EndTabBar();
			// Update history when the active tab changes; skip frames where a tab was closed
			// to avoid recording a stale index that no longer maps to the same document
			if (!anyTabClosed && renderedThisFrame != -1 && renderedThisFrame != lastRenderedDocumentIndex) {
				previousDocumentIndex = lastRenderedDocumentIndex;
				lastRenderedDocumentIndex = renderedThisFrame;
			}
		}
	}

} // namespace RetrodevGui
