// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Main view -- project panel (SDK, Project, Files trees).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <string>
#include <vector>
#include <filesystem>

namespace RetrodevGui {

	struct FileTreeNode {
		std::string name;
		std::filesystem::path fullPath;
		std::vector<FileTreeNode> children;
		bool isDirectory = false;
		bool isOpen = false;
		bool inProject = false;
		bool isRoot = false;
		bool isBuildItem = false;
		//
		// Virtual path within the Build section
		// For leaf build items: the full item path (e.g. "amstrad.cpc/tiles")
		// For intermediate folder nodes: the folder path (e.g. "amstrad.cpc", "maps")
		// For the Build root and nodes outside the Build section: empty
		//
		std::string buildItemPath;
		//
		// Build item type (only used when isBuildItem is true)
		//
		RetrodevLib::ProjectBuildType buildItemType = RetrodevLib::ProjectBuildType::Bitmap;
		//
		// True for the Build root node and every node inside it (folders and leaf items)
		//
		bool inBuildSection = false;
		//
		// True when this directory node is an explicitly-created virtual folder stored in buildFolders.
		// False for implicit intermediate folder nodes auto-generated from build item path segments.
		//
		bool isVirtualFolder = false;
	};

	class ProjectView {
	public:
		static void Perform();
		//
		// Notify the project view that the project has changed (e.g., item renamed)
		// This triggers a refresh of the tree on the next frame
		//
		static void NotifyProjectChanged();

		// Get directory contents recursively and build a tree structure
		static FileTreeNode GetDirectoryTree(const std::filesystem::path& path);

	private:
		// Recursively populate the tree node with directory contents
		static void PopulateTreeNode(FileTreeNode& node);

		//
		// Populate the Build node with build items from the project
		//
		static void PopulateBuildNode(FileTreeNode& node);
		//
		// Mark directory nodes whose virtual path is in the explicit folders list
		//
		static void MarkVirtualFolders(FileTreeNode& node, const std::vector<std::string>& folders);
		//
		// Add a build item to the tree, creating intermediate folders as needed
		//
		static void AddBuildItemToTree(FileTreeNode& parent, const std::string& itemPath, bool isBuildItem, RetrodevLib::ProjectBuildType buildType);

		// Expand parent folders to make a build item visible
		static bool ExpandPathToItem(FileTreeNode& node, const std::string& targetItemName);

		// Render a tree node in ImGui
		static void RenderTreeNode(FileTreeNode& node, ImGuiTreeNodeFlags baseFlags);

		// Render context menu for a tree node (extracting actions to a separate function)
		static void RenderContextMenu(FileTreeNode& node);

		// Get the icon for a file based on its extension
		static const char* GetFileIcon(const std::filesystem::path& path);

		// Get the project file type based on its extension
		static RetrodevLib::ProjectFileType GetFileType(const std::filesystem::path& path);
	};

}