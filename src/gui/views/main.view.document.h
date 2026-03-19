// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <string>

namespace RetrodevGui {

	class DocumentView {
	public:
		DocumentView(const std::string& name, const std::string& filepath) : m_name(name), m_filePath(filepath), m_modified(false) {}
		virtual ~DocumentView() = default;

		// Render the document content (called from DocumentsView)
		virtual void Perform() = 0;
		//
		// Save the document content to disk.
		// Override in subclasses that own their own file (e.g. text documents).
		// Project-backed documents (bitmaps, build items, maps) rely on Project::Save()
		// and return true here without doing any work.
		//
		virtual bool Save() { return true; }
		//
		// Get the build type this document represents
		// Returns None for non-build documents (like DocumentImage)
		//
		virtual RetrodevLib::ProjectBuildType GetBuildType() const { return RetrodevLib::ProjectBuildType::Bitmap; }

		// Document name (displayed in the tab)
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& name) { m_name = name; }

		// Get the absolute file path of the document
		const std::string& GetFilePath() const { return m_filePath; }
		//
		// Project-relative path using forward slashes (e.g. "src/main.asm").
		// Set once after construction; used for log navigation matching.
		//
		const std::string& GetProjectRelativePath() const { return m_projectRelativePath; }
		void SetProjectRelativePath(const std::string& relPath) { m_projectRelativePath = relPath; }

		// Whether the document has unsaved modifications
		bool IsModified() const { return m_modified; }
		void SetModified(bool modified) { m_modified = modified; }
		//
		// Scroll the document to the given line and place the cursor there.
		// Only meaningful for text documents; other document types ignore this.
		//
		virtual void ScrollToLine(int line) { (void)line; }
		//
		// Called when a project build item has been modified externally.
		// Subclasses override this to refresh state that depends on the named item.
		//
		virtual void OnProjectItemChanged(const std::string& itemName) { (void)itemName; }

	protected:
		std::string m_name;
		std::string m_filePath;
		std::string m_projectRelativePath;
		bool m_modified;
	};

} // namespace RetrodevGui