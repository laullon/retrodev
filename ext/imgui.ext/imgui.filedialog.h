//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

//
// Based on: https://github.com/dfranx/ImFileDialog
// Heavily modified
//

#pragma once
#include <ctime>
#include <stack>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <unordered_map>
#include <algorithm>

namespace ImGui {
	//
	// Modal file dialog supporting open (single/multi-select), directory picker and save modes.
	// Renders as an ImGui popup modal; call IsDone() every frame to drive it.
	// Usage pattern:
	//   Open/Save    — call once to open, call IsDone() every frame, read result when true.
	//   Singleton    — access via FileDialog::Instance().
	//
	class FileDialog {
	public:
		//
		// Dialog mode: determines filtering behaviour and the label of the confirm button.
		//   File      — open one or more existing files (filter applied).
		//   Directory — open an existing directory (filter ignored).
		//   Save      — choose a save path; the file need not exist yet.
		//
		enum class Type { File, Directory, Save };

		// Returns the process-wide singleton instance.
		static inline FileDialog& Instance() {
			static FileDialog ret;
			return ret;
		}

		FileDialog();
		~FileDialog();

		//
		// Opens a save dialog. Only one dialog may be open at a time; returns false if
		// another dialog is already active.
		//   key        — unique identifier used with IsDone() / HasResult().
		//   title      — window title shown in the title bar.
		//   filter     — extension filter string (e.g. "Images{.png,.jpg}").
		//   startingDir — optional initial directory; defaults to the last used directory.
		//
		bool Save(const std::string& key, const std::string& title, const std::string& filter, const std::string& startingDir = "");

		//
		// Opens a file or directory picker dialog. Only one dialog may be open at a time;
		// returns false if another dialog is already active.
		//   key          — unique identifier used with IsDone() / HasResult().
		//   title        — window title shown in the title bar.
		//   filter       — extension filter string; pass an empty string for directory mode.
		//   isMultiselect — allow the user to select multiple files (ignored in directory mode).
		//   startingDir  — optional initial directory; defaults to the last used directory.
		//
		bool Open(const std::string& key, const std::string& title, const std::string& filter, bool isMultiselect = false, const std::string& startingDir = "");

		//
		// Must be called every frame while the dialog is open. Opens the popup on the first
		// call and renders it on subsequent frames. Returns true once the user confirms or
		// cancels. After it returns true, call HasResult() / GetResult() / GetResults() to
		// read the outcome, then call Close().
		//
		bool IsDone(const std::string& key);

		// Returns true if the user confirmed and at least one path was selected.
		inline bool HasResult() { return m_result.size(); }
		// Returns the first (or only) selected path. Only valid when HasResult() is true.
		inline const std::filesystem::path& GetResult() { return m_result[0]; }
		// Returns all selected paths. Only valid when HasResult() is true.
		inline const std::vector<std::filesystem::path>& GetResults() { return m_result; }

		// Closes the dialog and clears internal navigation history. Call after consuming the result.
		void Close();

		// Removes a path from the Quick Access favourites list and the sidebar.
		void RemoveFavorite(const std::string& path);
		// Adds a path to the Quick Access favourites list and the sidebar. No-op if already present or the path does not exist.
		void AddFavorite(const std::string& path);
		// Returns the current favourites list.
		inline const std::vector<std::string>& GetFavorites() { return m_favorites; }

		// Sets the thumbnail zoom level, clamped to [1, 25].
		inline void SetZoom(float z) { m_zoom = std::min<float>(25.0f, std::max<float>(1.0f, z)); }
		// Returns the current thumbnail zoom level.
		inline float GetZoom() { return m_zoom; }

		// Font icon code-point for the generic file icon.
		std::string DefaultFileIcon;
		// Font icon code-point for the closed folder icon.
		std::string FolderIcon;
		// Font icon code-point for the open folder icon.
		std::string FolderOpenIcon;
		// Maps file extensions (e.g. ".png") to font icon code-points.
		std::unordered_map<std::string, std::string> FileTypeIcons;
		// When true, hidden files and directories (dot-prefixed on POSIX, hidden attribute on Windows) are shown.
		bool ShowHiddenFiles;

		//
		// Internal node used to build the left-panel directory tree.
		//
		class FileTreeNode {
		public:
			FileTreeNode(const std::string& path) {
				Path = std::filesystem::path(path);
				Read = false;
			}

			std::filesystem::path Path;
			// True once Children has been populated by a directory scan.
			bool Read;
			std::vector<FileTreeNode*> Children;
		};

		//
		// Metadata for a single entry in the current directory listing.
		//
		class FileData {
		public:
			FileData(const std::filesystem::path& path);

			std::filesystem::path Path;
			bool IsDirectory;
			size_t Size;
			time_t DateModified;
		};

	private:
		std::string m_currentKey;
		std::string m_currentTitle;
		std::filesystem::path m_currentDirectory;
		bool m_isMultiselect;
		bool m_isOpen;
		Type m_type;
		char m_inputTextbox[1024];
		char m_pathBuffer[1024];
		char m_newEntryBuffer[1024];
		char m_searchBuffer[128];
		std::vector<std::string> m_favorites;
		bool m_calledOpenPopup;
		std::stack<std::filesystem::path> m_backHistory, m_forwardHistory;
		float m_zoom;

		std::vector<std::filesystem::path> m_selections;
		int m_selectedFileItem;
		void m_select(const std::filesystem::path& path, bool isCtrlDown = false);

		std::vector<std::filesystem::path> m_result;
		bool m_finalize(const std::string& filename = "");

		std::string m_filter;
		std::vector<std::vector<std::string>> m_filterExtensions;
		size_t m_filterSelection;
		void m_parseFilter(const std::string& filter);

		const std::string& m_getIconFont(const std::filesystem::path& path);
		bool m_isHidden(const std::filesystem::path& path);

		std::vector<FileTreeNode*> m_treeCache;
		void m_clearTree(FileTreeNode* node);
		void m_renderTree(FileTreeNode* node);
		bool m_treeSyncRequired;

		unsigned int m_sortColumn;
		unsigned int m_sortDirection;
		std::vector<FileData> m_content;
		void m_setDirectory(const std::filesystem::path& p, bool addHistory = true);
		void m_sortContent(unsigned int column, unsigned int sortDirection);
		void m_renderContent();

		void m_renderPopups();
		void m_renderFileDialog();
	};
} // namespace ImGui
