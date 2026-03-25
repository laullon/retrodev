// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Main view -- documents panel (open editors tabbed area).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include "main.view.document.h"
#include <string>
#include <vector>
#include <memory>

namespace RetrodevGui {

	class DocumentsView {
	public:
		static void Perform();

		// Open a document tab
		static void OpenDocument(std::shared_ptr<DocumentView> document);

		// Close a document tab by index
		static void CloseDocument(int index);

		// Find and activate a document by name and file path (returns true if found and activated)
		static bool ActivateDocument(const std::string& name, const std::string& filePath);
		//
		// Find, activate and scroll a document to the given line (returns true if found and activated)
		//
		static bool ActivateDocumentAtLine(const std::string& name, const std::string& filePath, int line);
		//
		// Find and activate a document by name, file path, and build type (returns true if found and activated)
		// This is needed when multiple build items (bitmap, tileset, sprite) share the same source file
		//
		static bool ActivateDocument(const std::string& name, const std::string& filePath, RetrodevLib::ProjectBuildType buildType);

		// Check if a document with the given name exists (returns true if found)
		static bool IsDocumentNameOpen(const std::string& name);
		//
		// Returns true if any open document has unsaved modifications.
		// Used by the quit handler to decide whether to show a confirmation dialog.
		//
		static bool HasAnyModifiedDocuments();
		//
		// Clear the modified flag for all open documents
		// Called when the project is saved
		//
		static void ClearAllModifiedFlags();
		//
		// Save all open documents that have unsaved modifications.
		// Called before a build to ensure all file-backed documents (e.g. text files) are flushed to disk.
		//
		static void SaveAllModified();
		//
		// Close all open documents and reset the active document index
		// Called when the project is closed
		//
		static void CloseAllDocuments();
		//
		// Compute the forward-slash project-relative path for a given absolute file path.
		// Returns an empty string if no project is open or the path cannot be made relative.
		//
		static std::string ComputeProjectRelativePath(const std::string& absFilePath);
		//
		// Notify all open documents that a project build item has changed.
		// Each document decides independently whether the change is relevant to it.
		//
		static void NotifyProjectItemChanged(const std::string& itemName);
		//
		// Update the name of an open build item document after it has been renamed in the project.
		// Matches by the old name and build type; no-op if no such document is open.
		//
		static void RenameBuildItemDocument(const std::string& oldName, const std::string& newName, RetrodevLib::ProjectBuildType buildType);
		//
		// Close an open build item document by name and build type.
		// No-op if no matching document is open.
		//
		static void CloseBuildItemDocument(const std::string& name, RetrodevLib::ProjectBuildType buildType);

	private:
		// List of opened documents
		inline static std::vector<std::shared_ptr<DocumentView>> documents;
		// Currently active document index
		inline static int activeDocumentIndex = -1;
		// Index of the previously active document, used for Ctrl+Tab switching
		inline static int previousDocumentIndex = -1;
		// Which tab ImGui last rendered as active, used to detect tab changes
		inline static int lastRenderedDocumentIndex = -1;
		//
		// Pointer to the document currently being rendered; used to prevent self-notification
		// via NotifyProjectItemChanged when a document triggers a change to itself mid-render.
		//
		inline static DocumentView* s_performingDocument = nullptr;
		//
		// Name of the document pending close confirmation.
		// Set when the user clicks the tab close button on a modified document;
		// cleared once the confirm dialog resolves (either direction).
		//
		inline static std::string s_pendingCloseDocumentName;
		//
		// Close a set of tabs by index (sorted ascending), warning once for all unsaved ones.
		//
		static void CloseDocumentSet(std::vector<int> indicesToClose);
	};

}
