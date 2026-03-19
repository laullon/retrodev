// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "project.h"
#include <retrodev.lib.h>
#include <log/log.h>
#include "metadata/meta.project.h"
#include <glaze/glaze.hpp>
#include <filesystem>
#include <export/export.h>
#include <assets/palette/palette.h>

namespace RetrodevLib {
	//
	// Internal state for the currently open project
	//
	static std::string currentProjectPath;
	static std::string currentProjectName;
	static bool isProjectOpen = false;
	static bool isProjectModified = false;
	static ProjectFile currentProject;
	//
	// Absolute path to the SDK folder (set once at startup, empty if no SDK is present).
	// Used to store/expand the $(sdk) variable in project file paths.
	//
	static std::string s_sdkFolderPath;

	//
	// Prefix used in the project file to represent the SDK folder location.
	// Stored as a forward-slash path so the project file is portable.
	//
	static constexpr const char* k_sdkPrefix = "$(sdk)/";
	//
	// Util: Convert an absolute path to a stored project path.
	// If the file lives under the SDK folder it is collapsed to "$(sdk)/relative".
	// Otherwise it is stored relative to the project folder.
	// Always returns forward slashes so the result matches the JSON-stored paths.
	//
	static std::string toRelativePath(const std::string& filePath) {
		if (!s_sdkFolderPath.empty()) {
			std::error_code ec;
			std::filesystem::path rel = std::filesystem::relative(std::filesystem::path(filePath), std::filesystem::path(s_sdkFolderPath), ec);
			if (!ec && !rel.empty()) {
				std::string relStr = rel.generic_string();
				//
				// relative() returns "." when the paths are equal, and starts with ".." when
				// the file is outside the SDK folder — only collapse when it is truly inside.
				//
				if (relStr != "." && relStr.rfind("..", 0) != 0)
					return std::string(k_sdkPrefix) + relStr;
			}
		}
		std::filesystem::path projectFolder = std::filesystem::path(currentProjectPath).parent_path();
		std::string relative = std::filesystem::relative(std::filesystem::path(filePath), projectFolder).generic_string();
		return relative;
	}
	//
	// Util: Convert a stored project path to an absolute path.
	// Expands the "$(sdk)/" prefix to the actual SDK folder path.
	// For regular relative paths, resolves against the project folder.
	//
	static std::string toAbsolutePath(const std::string& storedPath) {
		if (storedPath.rfind(k_sdkPrefix, 0) == 0) {
			std::string rest = storedPath.substr(std::strlen(k_sdkPrefix));
			return (std::filesystem::path(s_sdkFolderPath) / rest).string();
		}
		std::filesystem::path projectFolder = std::filesystem::path(currentProjectPath).parent_path();
		std::filesystem::path absolutePath = projectFolder / storedPath;
		return absolutePath.string();
	}

	//
	// Set the SDK folder path (called once at startup when the sdk folder is detected).
	// An empty string means no SDK is available.
	//
	void Project::SetSdkFolder(const std::string& path) {
		s_sdkFolderPath = path;
	}
	//
	// Return the current SDK folder path (empty if not set).
	//
	std::string Project::GetSdkFolder() {
		return s_sdkFolderPath;
	}
	//
	// Expand a stored project path to an absolute filesystem path.
	// Resolves the $(sdk) variable and relative paths.
	// Returns the input unchanged if it is already absolute and contains no variable.
	//
	std::string Project::ExpandPath(const std::string& storedPath) {
		return toAbsolutePath(storedPath);
	}
	//
	// Collapse an absolute filesystem path to its portable stored form.
	// Delegates to toRelativePath which applies the $(sdk)/ prefix when appropriate.
	//
	std::string Project::CollapsePath(const std::string& absolutePath) {
		return toRelativePath(absolutePath);
	}
	//
	// Create a new project at the given path
	// It receives the absolute path to the project file (.retrodev),
	// initializes the project state, and sets the project name from the filename stem.
	//
	bool Project::New(const std::string& path) {
		currentProject = {};
		currentProjectPath = path;
		currentProjectName = std::filesystem::path(path).stem().string();
		currentProject.ProjectName = currentProjectName;
		isProjectOpen = true;
		isProjectModified = true; // New projects are considered modified (not saved yet)
		return true;
	}

	//
	// Open an existing project from the given path
	// It receives the absolute path to the project file (.retrodev),
	// reads and parses the JSON content, and initializes the project state.
	//
	bool Project::Open(const std::string& path) {
		if (!std::filesystem::exists(path)) {
			Log::Error("Project::Open file not found: '%s'", path.c_str());
			return false;
		}
		currentProject = {};
		std::string buffer;
		auto ec = glz::read_file_json<glz::opts{.error_on_missing_keys = false}>(currentProject, path, buffer);
		if (ec) {
			Log::Error("Project::Open failed to parse JSON: '%s'", path.c_str());
			currentProject = {};
			return false;
		}
		currentProjectPath = path;
		currentProjectName = currentProject.ProjectName;
		isProjectOpen = true;
		isProjectModified = false; // Just opened, no changes yet
		return true;
	}

	//
	// Save the current project to the given path
	//
	bool Project::Save(const std::string& path) {
		if (!isProjectOpen) {
			return false;
		}
		std::string buffer;
		auto ec = glz::write_json(currentProject, buffer);
		if (ec) {
			Log::Error("Project::Save failed to serialise project to JSON: '%s'", path.c_str());
			return false;
		}
		std::string pretty = glz::prettify_json(buffer);
		//
		// Normalise path separators to forward slashes so the project file is
		// portable across Windows and Linux without manual edits.
		// Only replace inside JSON string values: the backslash is always written
		// as "\\" in JSON, so replacing every "\\\\" sequence (two chars in the
		// actual string: backslash + backslash) with "/" is both safe and complete.
		//
		for (std::size_t i = 0; i < pretty.size(); i++) {
			if (pretty[i] == '\\' && i + 1 < pretty.size() && pretty[i + 1] == '\\') {
				pretty[i] = '/';
				pretty.erase(i + 1, 1);
			}
		}
		auto wec = glz::buffer_to_file(pretty, path);
		if (wec != glz::error_code::none) {
			Log::Error("Project::Save failed to write file: '%s'", path.c_str());
			return false;
		}
		currentProjectPath = path;
		isProjectModified = false; // Saved successfully, clear modified flag
		return true;
	}

	//
	// Close the current project
	//
	bool Project::Close() {
		currentProject = {};
		currentProjectPath.clear();
		currentProjectName.clear();
		isProjectOpen = false;
		isProjectModified = false;
		return true;
	}

	//
	// Check if a project is currently open
	//
	bool Project::IsOpen() {
		return isProjectOpen;
	}

	//
	// Check whether the project has unsaved changes
	//
	bool Project::IsModified() {
		return isProjectModified;
	}

	//
	// Mark the project as modified (has unsaved changes)
	//
	void Project::MarkAsModified() {
		isProjectModified = true;
	}

	//
	// Clear the modified flag (after saving)
	//
	void Project::ClearModified() {
		isProjectModified = false;
	}

	//
	// Get the current project path
	//
	std::string Project::GetPath() {
		return currentProjectPath;
	}

	//
	// Get the current project name
	//
	std::string Project::GetName() {
		return currentProjectName;
	}

	//
	// Add a file to the current project.
	// It receives the absolute path, but stores it as a relative path to the project folder.
	//
	bool Project::AddFile(const std::string& filePath, ProjectFileType type) {
		if (!isProjectOpen) {
			return false;
		}
		std::string relativePath = toRelativePath(filePath);
		switch (type) {
			case ProjectFileType::Image:
				for (const auto& entry : currentProject.images) {
					if (entry.filePath == relativePath)
						return false;
				}
				currentProject.images.push_back({relativePath});
				isProjectModified = true;
				break;
			case ProjectFileType::Audio:
				for (const auto& entry : currentProject.audio) {
					if (entry.filePath == relativePath)
						return false;
				}
				currentProject.audio.push_back({relativePath});
				isProjectModified = true;
				break;
			case ProjectFileType::Source:
				for (const auto& entry : currentProject.sources) {
					if (entry.filePath == relativePath)
						return false;
				}
				currentProject.sources.push_back({relativePath});
				isProjectModified = true;
				break;
			case ProjectFileType::Script:
				for (const auto& entry : currentProject.scripts) {
					if (entry.filePath == relativePath)
						return false;
				}
				currentProject.scripts.push_back({relativePath});
				isProjectModified = true;
				break;
			case ProjectFileType::Data:
				for (const auto& entry : currentProject.data) {
					if (entry.filePath == relativePath)
						return false;
				}
				currentProject.data.push_back({relativePath});
				isProjectModified = true;
				break;
		}
		return true;
	}

	//
	// Remove a file from the current project
	// It receives the absolute path, converts it to a relative path, and searches for it in all file type categories.
	// If found, it removes the entry and also removes any build items that reference this file as a data source.
	//
	bool Project::RemoveFile(const std::string& filePath) {
		if (!isProjectOpen) {
			return false;
		}
		std::string relativePath = toRelativePath(filePath);
		bool removed = false;
		// Remove from file lists
		for (auto it = currentProject.images.begin(); it != currentProject.images.end(); ++it) {
			if (it->filePath == relativePath) {
				currentProject.images.erase(it);
				removed = true;
				break;
			}
		}
		if (!removed) {
			for (auto it = currentProject.audio.begin(); it != currentProject.audio.end(); ++it) {
				if (it->filePath == relativePath) {
					currentProject.audio.erase(it);
					removed = true;
					break;
				}
			}
		}
		if (!removed) {
			for (auto it = currentProject.sources.begin(); it != currentProject.sources.end(); ++it) {
				if (it->filePath == relativePath) {
					currentProject.sources.erase(it);
					removed = true;
					break;
				}
			}
		}
		if (!removed) {
			for (auto it = currentProject.scripts.begin(); it != currentProject.scripts.end(); ++it) {
				if (it->filePath == relativePath) {
					currentProject.scripts.erase(it);
					removed = true;
					break;
				}
			}
		}
		if (!removed) {
			for (auto it = currentProject.data.begin(); it != currentProject.data.end(); ++it) {
				if (it->filePath == relativePath) {
					currentProject.data.erase(it);
					removed = true;
					break;
				}
			}
		}
		if (removed)
			isProjectModified = true;
		return removed;
	}

	//
	// Get the folder containing the current project file.
	// Just the path where the project file is located, without the filename. Returns empty string if no project is open.
	//
	std::string Project::GetProjectFolder() {
		if (!isProjectOpen || currentProjectPath.empty()) {
			return "";
		}
		return std::filesystem::path(currentProjectPath).parent_path().string();
	}

	//
	// Check if a file is part of the current project
	// It only checks if the file is tracked in any of the file type categories (images, audio, sources, data).
	// It does not check build items.
	//
	bool Project::IsFileInProject(const std::string& filePath) {
		if (!isProjectOpen) {
			return false;
		}
		std::string relativePath = toRelativePath(filePath);
		for (const auto& entry : currentProject.images) {
			if (entry.filePath == relativePath)
				return true;
		}
		for (const auto& entry : currentProject.audio) {
			if (entry.filePath == relativePath)
				return true;
		}
		for (const auto& entry : currentProject.sources) {
			if (entry.filePath == relativePath)
				return true;
		}
		for (const auto& entry : currentProject.scripts) {
			if (entry.filePath == relativePath)
				return true;
		}
		for (const auto& entry : currentProject.data) {
			if (entry.filePath == relativePath)
				return true;
		}
		return false;
	}

	//
	// Get the list of build item names for a given build type category
	// It receives the build type (e.g. Bitmap, Tilemap, Sprite) and returns a vector of build item names for that type.
	// Returns an empty vector if no project is open.
	// The returned vector contains the build item names, which are unique identifiers for the build items
	// and can be used to retrieve their configurations or source file paths.
	//
	std::vector<std::string> Project::GetBuildItemsByType(ProjectBuildType type) {
		std::vector<std::string> result;
		if (!isProjectOpen) {
			return result;
		}
		//
		// Select the appropriate build list based on type
		//
		switch (type) {
			case ProjectBuildType::Bitmap:
				for (const auto& entry : currentProject.buildBitmaps) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::Tilemap:
				for (const auto& entry : currentProject.buildTiles) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::Sprite:
				for (const auto& entry : currentProject.buildSprites) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::Map:
				for (const auto& entry : currentProject.buildMaps) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::Build:
				for (const auto& entry : currentProject.buildBuilds) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::Palette:
				for (const auto& entry : currentProject.buildPalettes) {
					result.push_back(entry.name);
				}
				break;
			case ProjectBuildType::VirtualFolder:
				break;
		}
		return result;
	}

	//
	// Return the relative paths of all source files tracked in the project
	//
	std::vector<std::string> Project::GetSourceFiles() {
		std::vector<std::string> result;
		if (!isProjectOpen)
			return result;
		for (const auto& entry : currentProject.sources)
			result.push_back(entry.filePath);
		return result;
	}

	//
	// Return the unique relative folder paths derived from all tracked project files
	//
	std::vector<std::string> Project::GetProjectFolders() {
		std::vector<std::string> result;
		if (!isProjectOpen)
			return result;
		//
		// Always offer the project root
		//
		result.push_back(".");
		//
		// Collect parent directories from every tracked file collection
		//
		auto addParent = [&](const std::string& relPath) {
			std::string parent = std::filesystem::path(relPath).parent_path().generic_string();
			if (parent.empty())
				return;
			for (const auto& existing : result)
				if (existing == parent)
					return;
			result.push_back(parent);
		};
		for (const auto& e : currentProject.images)
			addParent(e.filePath);
		for (const auto& e : currentProject.audio)
			addParent(e.filePath);
		for (const auto& e : currentProject.sources)
			addParent(e.filePath);
		for (const auto& e : currentProject.scripts)
			addParent(e.filePath);
		for (const auto& e : currentProject.data)
			addParent(e.filePath);
		return result;
	}

	//
	// Return the absolute paths of all AngelScript files tracked in the project
	//
	std::vector<std::string> Project::GetScriptFiles() {
		std::vector<std::string> result;
		if (!isProjectOpen)
			return result;
		for (const auto& entry : currentProject.scripts)
			result.push_back(toAbsolutePath(entry.filePath));
		return result;
	}
	//
	// Scan the project folder recursively and return relative paths of files
	// whose lowercased extension (without dot) matches any entry in extensions.
	//
	std::vector<std::string> Project::GetFilesByExtensions(const std::vector<std::string>& extensions) {
		std::vector<std::string> result;
		if (!isProjectOpen || currentProjectPath.empty())
			return result;
		std::filesystem::path projectFolder = std::filesystem::path(currentProjectPath).parent_path();
		std::error_code ec;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectFolder, ec)) {
			if (!entry.is_regular_file())
				continue;
			std::string ext = entry.path().extension().string();
			if (!ext.empty() && ext[0] == '.')
				ext = ext.substr(1);
			for (auto& c : ext)
				c = (char)std::tolower((unsigned char)c);
			for (const auto& wanted : extensions) {
				if (ext == wanted) {
					std::filesystem::path rel = std::filesystem::relative(entry.path(), projectFolder, ec);
					if (!ec)
						result.push_back(rel.generic_string());
					break;
				}
			}
		}
		return result;
	}
	//
	// Cached snapshot of all relative file paths seen on disk during the last scan.
	// Used to detect new files appearing or existing files disappearing so the tree
	// can be rebuilt even for untracked entries.
	//
	static std::vector<std::string> s_lastDiskSnapshot;
	//
	// Scan the project folder and tracked lists to reflect filesystem changes:
	//   - Removes tracked entries whose file no longer exists on disk.
	//   - Detects any change in the set of files present on disk (new files, deleted files)
	//     by comparing against a cached snapshot, so the caller can rebuild the tree.
	// Uses error_code overloads so a transient I/O failure does not cause false positives.
	//
	bool Project::ScanFiles() {
		if (!isProjectOpen || currentProjectPath.empty())
			return false;
		bool changed = false;
		std::filesystem::path projectFolder = std::filesystem::path(currentProjectPath).parent_path();
		//
		// Remove tracked entries whose file is definitively absent (exists() returns false with no error).
		// If exists() sets an error code the entry is left untouched.
		//
		auto removeStale = [&](auto& collection) {
			for (auto it = collection.begin(); it != collection.end();) {
				std::error_code ec;
				//
				// Resolve stored path to absolute (handles both $(sdk)/... and relative paths)
				//
				std::filesystem::path abs = std::filesystem::path(toAbsolutePath(it->filePath));
				bool exists = std::filesystem::exists(abs, ec);
				if (!ec && !exists) {
					it = collection.erase(it);
					changed = true;
				} else {
					++it;
				}
			}
		};
		removeStale(currentProject.images);
		removeStale(currentProject.audio);
		removeStale(currentProject.sources);
		removeStale(currentProject.scripts);
		removeStale(currentProject.data);
		if (changed)
			isProjectModified = true;
		//
		// Build a fresh snapshot of all regular files currently on disk.
		//
		std::vector<std::string> currentSnapshot;
		std::error_code ec;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(projectFolder, ec)) {
			if (!entry.is_regular_file())
				continue;
			std::filesystem::path rel = std::filesystem::relative(entry.path(), projectFolder, ec);
			if (!ec)
				currentSnapshot.push_back(rel.generic_string());
		}
		std::sort(currentSnapshot.begin(), currentSnapshot.end());
		//
		// If the snapshot differs from the last one, the tree must be rebuilt.
		//
		if (currentSnapshot != s_lastDiskSnapshot) {
			s_lastDiskSnapshot = std::move(currentSnapshot);
			changed = true;
		}
		return changed;
	}
	//
	// Rename a build item in the current project
	// Generic function that delegates to the specific rename function based on build type
	// This provides a unified interface for renaming build items regardless of their type
	//
	bool Project::RenameBuildItem(ProjectBuildType type, const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		//
		//
		// Delegate to the specific rename function based on build type
		//
		switch (type) {
			case ProjectBuildType::Bitmap:
				return BitmapRename(oldName, newName);
			case ProjectBuildType::Tilemap:
				return TilesetRename(oldName, newName);
			case ProjectBuildType::Sprite:
				return SpriteRename(oldName, newName);
			case ProjectBuildType::Map:
				return MapRename(oldName, newName);
			case ProjectBuildType::Build:
				return BuildRename(oldName, newName);
			case ProjectBuildType::Palette:
				return PaletteRename(oldName, newName);
			default:
				return false;
		}
	}

	//
	// Get the source file path for a bitmap build item
	//
	std::string Project::BitmapGetSourcePath(const std::string& name) {
		if (!isProjectOpen)
			return "";
		//
		// Find bitmap by name and return its source file path
		//
		for (const auto& entry : currentProject.buildBitmaps) {
			if (entry.name == name)
				return toAbsolutePath(entry.sourceFilePath);
		}
		return "";
	}

	//
	// Add a bitmap conversion build item to the current project
	// It receives the bitmap name to use and the absolute path to the image source to be used
	//
	bool Project::BitmapAdd(const std::string& name, const std::string& sourceFilePath) {
		if (!isProjectOpen)
			return false;
		// Check if bitmap name already exists
		for (const auto& entry : currentProject.buildBitmaps) {
			if (entry.name == name)
				return false;
		}
		// Convert source file path to relative path
		std::string relativePath = toRelativePath(sourceFilePath);
		// Add new bitmap build item
		GFXParams defaultParams = {}; // Use default conversion parameters for now
		currentProject.buildBitmaps.push_back({name, relativePath, defaultParams, {}});
		isProjectModified = true;
		return true;
	}

	//
	// Remove a bitmap conversion build item from the current project.
	// Returns false if no project is open or nothing was removed.
	// nameOrSourcePath: bitmap name to remove (exact match), or source file path (removes all bitmaps using that source)
	//
	bool Project::BitmapRemove(const std::string& nameOrSourcePath) {
		if (!isProjectOpen)
			return false;
		bool removed = false;
		// First, try to find and remove by exact name match
		for (auto it = currentProject.buildBitmaps.begin(); it != currentProject.buildBitmaps.end(); ++it) {
			if (it->name == nameOrSourcePath) {
				currentProject.buildBitmaps.erase(it);
				isProjectModified = true;
				return true;
			}
		}
		// If not found by name, convert to relative path and remove all bitmaps using that source file
		std::string relativePath = toRelativePath(nameOrSourcePath);
		for (auto it = currentProject.buildBitmaps.begin(); it != currentProject.buildBitmaps.end();) {
			if (it->sourceFilePath == relativePath) {
				it = currentProject.buildBitmaps.erase(it);
				removed = true;
			} else {
				++it;
			}
		}
		if (removed) {
			isProjectModified = true;
		}
		return removed;
	}

	//
	// Return a pointer the configuration for a given name if it exists
	// true if the pointer is valid and configuration was found. False otherwise
	//
	bool Project::BitmapGetCfg(const std::string& name, GFXParams** config) {
		if (!isProjectOpen)
			return false;
		// First, try to find and remove by exact name match
		for (auto it = currentProject.buildBitmaps.begin(); it != currentProject.buildBitmaps.end(); ++it) {
			if (it->name == name) {
				*config = &it->params;
				return true;
			}
		}
		return false;
	}

	//
	// Rename a bitmap conversion build item in the current project.
	// Returns false if no project is open, the old name does not exist, or the new name already exists.
	bool Project::BitmapRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		// Check if new name already exists
		for (const auto& entry : currentProject.buildBitmaps) {
			if (entry.name == newName)
				return false;
		}
		//
		// Find bitmap by old name and rename it
		//
		for (auto& entry : currentProject.buildBitmaps) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get the source file path for a tileset build item
	//
	std::string Project::TilesetGetSourcePath(const std::string& name) {
		if (!isProjectOpen)
			return "";
		//
		// Find tileset by name and return its source file path
		//
		for (const auto& entry : currentProject.buildTiles) {
			if (entry.name == name)
				return toAbsolutePath(entry.sourceFilePath);
		}
		return "";
	}
	//
	// Add a tileset conversion build item to the current project
	// It receives the tileset name to use and the absolute path to the image source to be used
	//
	bool Project::TilesetAdd(const std::string& name, const std::string& sourceFilePath) {
		if (!isProjectOpen)
			return false;
		//
		// Check if tileset name already exists
		//
		for (const auto& entry : currentProject.buildTiles) {
			if (entry.name == name)
				return false;
		}
		//
		// Convert source file path to relative path
		//
		std::string relativePath = toRelativePath(sourceFilePath);
		//
		// Add new tileset build item with default conversion parameters
		//
		GFXParams defaultParams = {};
		ProjectBuildTilesEntry newTileset;
		newTileset.name = name;
		newTileset.sourceFilePath = relativePath;
		newTileset.params = defaultParams;
		currentProject.buildTiles.push_back(newTileset);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a tileset conversion build item from the current project.
	// Returns false if no project is open or nothing was removed.
	// nameOrSourcePath: tileset name to remove (exact match), or source file path (removes all tilesets using that source)
	//
	bool Project::TilesetRemove(const std::string& nameOrSourcePath) {
		if (!isProjectOpen)
			return false;
		bool removed = false;
		//
		// First, try to find and remove by exact name match
		//
		for (auto it = currentProject.buildTiles.begin(); it != currentProject.buildTiles.end(); ++it) {
			if (it->name == nameOrSourcePath) {
				currentProject.buildTiles.erase(it);
				isProjectModified = true;
				return true;
			}
		}
		//
		// If not found by name, convert to relative path and remove all tilesets using that source file
		//
		std::string relativePath = toRelativePath(nameOrSourcePath);
		for (auto it = currentProject.buildTiles.begin(); it != currentProject.buildTiles.end();) {
			if (it->sourceFilePath == relativePath) {
				it = currentProject.buildTiles.erase(it);
				removed = true;
			} else {
				++it;
			}
		}
		if (removed) {
			isProjectModified = true;
		}
		return removed;
	}
	//
	// Get configuration for a tileset build item
	// Returns the GFXParams for the tileset (same structure as bitmaps for now)
	//
	bool Project::TilesetGetCfg(const std::string& name, GFXParams** config) {
		if (!isProjectOpen)
			return false;
		//
		// Find tileset by name and return pointer to its params
		//
		for (auto& entry : currentProject.buildTiles) {
			if (entry.name == name) {
				if (config != nullptr)
					*config = &entry.params;
				return true;
			}
		}
		return false;
	}
	//
	// Get tile extraction parameters for a tileset build item
	//
	bool Project::TilesetGetTileParams(const std::string& name, TileExtractionParams** tileParams) {
		if (!isProjectOpen)
			return false;
		//
		// Find tileset by name and return pointer to its tile params
		//
		for (auto& entry : currentProject.buildTiles) {
			if (entry.name == name) {
				if (tileParams != nullptr)
					*tileParams = &entry.tileParams;
				return true;
			}
		}
		return false;
	}
	//
	// Rename a tileset conversion build item in the current project
	//
	bool Project::TilesetRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		//
		// Check if new name already exists
		//
		for (const auto& entry : currentProject.buildTiles) {
			if (entry.name == newName)
				return false;
		}
		//
		// Find tileset by old name and rename it
		//
		for (auto& entry : currentProject.buildTiles) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get the source file path for a sprite build item
	//
	std::string Project::SpriteGetSourcePath(const std::string& name) {
		if (!isProjectOpen)
			return "";
		//
		// Find sprite by name and return absolute path to source file
		//
		for (const auto& entry : currentProject.buildSprites) {
			if (entry.name == name)
				return toAbsolutePath(entry.sourceFilePath);
		}
		return "";
	}
	//
	// Add a sprite conversion build item to the current project
	//
	bool Project::SpriteAdd(const std::string& name, const std::string& sourceFilePath) {
		if (!isProjectOpen)
			return false;
		//
		// Check if name already exists
		//
		for (const auto& entry : currentProject.buildSprites) {
			if (entry.name == name)
				return false;
		}
		//
		// Convert source file path to relative path
		//
		std::string relativePath = toRelativePath(sourceFilePath);
		//
		// Add new sprite build item with default conversion parameters
		//
		GFXParams defaultParams = {};
		ProjectBuildSpriteEntry newSprite;
		newSprite.name = name;
		newSprite.sourceFilePath = relativePath;
		newSprite.params = defaultParams;
		currentProject.buildSprites.push_back(newSprite);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a sprite conversion build item from the current project
	//
	bool Project::SpriteRemove(const std::string& nameOrSourcePath) {
		if (!isProjectOpen)
			return false;
		bool removed = false;
		//
		// First, try to find and remove by exact name match
		//
		for (auto it = currentProject.buildSprites.begin(); it != currentProject.buildSprites.end(); ++it) {
			if (it->name == nameOrSourcePath) {
				currentProject.buildSprites.erase(it);
				isProjectModified = true;
				return true;
			}
		}
		//
		// If not found by name, convert to relative path and remove all sprites using that source file
		//
		std::string relativePath = toRelativePath(nameOrSourcePath);
		for (auto it = currentProject.buildSprites.begin(); it != currentProject.buildSprites.end();) {
			if (it->sourceFilePath == relativePath) {
				it = currentProject.buildSprites.erase(it);
				removed = true;
			} else {
				++it;
			}
		}
		if (removed) {
			isProjectModified = true;
		}
		return removed;
	}
	//
	// Get configuration for a sprite build item (placeholder - uses GFXParams temporarily)
	//
	bool Project::SpriteGetCfg(const std::string& name, GFXParams** config) {
		if (!isProjectOpen)
			return false;
		//
		// Find sprite by name and return pointer to its params
		//
		for (auto& entry : currentProject.buildSprites) {
			if (entry.name == name) {
				if (config != nullptr)
					*config = &entry.params;
				return true;
			}
		}
		return false;
	}
	//
	// Rename a sprite conversion build item in the current project
	//
	bool Project::SpriteRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		//
		// Check if new name already exists
		//
		for (const auto& entry : currentProject.buildSprites) {
			if (entry.name == newName)
				return false;
		}
		//
		// Find sprite by old name and rename it
		//
		for (auto& entry : currentProject.buildSprites) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get sprite extraction parameters for a sprite build item
	//
	bool Project::SpriteGetSpriteParams(const std::string& name, SpriteExtractionParams** spriteParams) {
		if (!isProjectOpen)
			return false;
		//
		// Find sprite by name and return pointer to its sprite params
		//
		for (auto& entry : currentProject.buildSprites) {
			if (entry.name == name) {
				if (spriteParams != nullptr)
					*spriteParams = &entry.spriteParams;
				return true;
			}
		}
		return false;
	}
	//
	// Add a map build item to the current project
	//
	bool Project::MapAdd(const std::string& name) {
		if (!isProjectOpen)
			return false;
		//
		// Check if name already exists
		//
		for (const auto& entry : currentProject.buildMaps) {
			if (entry.name == name)
				return false;
		}
		//
		// Add new map build item with a default first layer
		//
		ProjectBuildMapEntry entry;
		entry.name = name;
		RetrodevLib::MapLayer defaultLayer;
		defaultLayer.name = "Layer 1";
		entry.mapParams.layers.push_back(std::move(defaultLayer));
		currentProject.buildMaps.push_back(entry);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a map build item from the current project
	//
	bool Project::MapRemove(const std::string& name) {
		if (!isProjectOpen)
			return false;
		//
		// Find and remove by exact name match
		//
		for (auto it = currentProject.buildMaps.begin(); it != currentProject.buildMaps.end(); ++it) {
			if (it->name == name) {
				currentProject.buildMaps.erase(it);
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get parameters for a map build item
	//
	bool Project::MapGetParams(const std::string& name, MapParams** params) {
		if (!isProjectOpen)
			return false;
		//
		// Find map by name and return pointer to its params
		//
		for (auto& entry : currentProject.buildMaps) {
			if (entry.name == name) {
				if (params != nullptr)
					*params = &entry.mapParams;
				return true;
			}
		}
		return false;
	}
	//
	// Rename a map build item in the current project
	//
	bool Project::MapRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		//
		// Check if new name already exists
		//
		for (const auto& entry : currentProject.buildMaps) {
			if (entry.name == newName)
				return false;
		}
		//
		// Find map by old name and rename it
		//
		for (auto& entry : currentProject.buildMaps) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a bitmap build item
	//
	bool Project::BitmapGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildBitmaps) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a tileset build item
	//
	bool Project::TilesetGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildTiles) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a sprite build item
	//
	bool Project::SpriteGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildSprites) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a map build item
	//
	bool Project::MapGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildMaps) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}
	//
	// Add a build script item to the current project
	//
	bool Project::BuildAdd(const std::string& name) {
		if (!isProjectOpen)
			return false;
		for (const auto& entry : currentProject.buildBuilds) {
			if (entry.name == name)
				return false;
		}
		ProjectBuildBuildEntry entry;
		entry.name = name;
		currentProject.buildBuilds.push_back(entry);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a build script item from the current project
	//
	bool Project::BuildRemove(const std::string& name) {
		if (!isProjectOpen)
			return false;
		for (auto it = currentProject.buildBuilds.begin(); it != currentProject.buildBuilds.end(); ++it) {
			if (it->name == name) {
				currentProject.buildBuilds.erase(it);
				//
				// Clear the selection if the removed item was the selected one
				//
				if (currentProject.selectedBuildItem == name)
					currentProject.selectedBuildItem.clear();
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get parameters for a build script item
	//
	bool Project::BuildGetParams(const std::string& name, SourceParams** params) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildBuilds) {
			if (entry.name == name) {
				if (params != nullptr)
					*params = &entry.sourceParams;
				return true;
			}
		}
		return false;
	}
	//
	// Rename a build script item in the current project
	//
	bool Project::BuildRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		for (const auto& entry : currentProject.buildBuilds) {
			if (entry.name == newName)
				return false;
		}
		for (auto& entry : currentProject.buildBuilds) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a build script item
	//
	bool Project::BuildGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildBuilds) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}
	//
	// Process all dependencies listed in the build item's SourceParams in order.
	// Each dependency is resolved by type (Bitmap, Sprite, Tileset, Map, Palette),
	// converted and exported as appropriate, with progress and errors logged on LogChannel::Build.
	//
	bool Project::BuildProcessDependencies(const std::string& name) {
		if (!isProjectOpen)
			return false;
		for (const auto& entry : currentProject.buildBuilds) {
			if (entry.name == name) {
				for (const auto& depName : entry.sourceParams.dependencies) {
					GFXParams* params = nullptr;
					//
					// Bitmap dependency: load image, create converter, convert and export
					//
					if (BitmapGetCfg(depName, &params) && params != nullptr) {
						Log::Info(LogChannel::Build, "[Dep] Bitmap '%s': loading source image.", depName.c_str());
						std::string sourcePath = BitmapGetSourcePath(depName);
						auto image = Image::ImageLoad(sourcePath);
						if (!image) {
							Log::Error(LogChannel::Build, "[Dep] Bitmap '%s': failed to load source image '%s'.", depName.c_str(), sourcePath.c_str());
							continue;
						}
						auto converter = Converters::GetBitmapConverter(params);
						if (!converter) {
							Log::Error(LogChannel::Build, "[Dep] Bitmap '%s': no suitable converter for the given parameters.", depName.c_str());
							continue;
						}
						converter->SetOriginal(image);
						Log::Info(LogChannel::Build, "[Dep] Bitmap '%s': converting.", depName.c_str());
						converter->Convert(params);
						auto converted = converter->GetConverted(params);
						if (!converted) {
							Log::Error(LogChannel::Build, "[Dep] Bitmap '%s': conversion produced no output.", depName.c_str());
							continue;
						}
						ExportParams* exportParams = nullptr;
						BitmapGetExportParams(depName, &exportParams);
						if (!exportParams || exportParams->scriptPath.empty()) {
							Log::Warning(LogChannel::Build, "[Dep] Bitmap '%s': no export script defined, skipping export.", depName.c_str());
							continue;
						}
						std::string absScript = ExpandPath(exportParams->scriptPath);
						std::string absOutput = ExpandPath(exportParams->outputName);
						Log::Info(LogChannel::Build, "[Dep] Bitmap '%s': exporting to '%s'.", depName.c_str(), absOutput.c_str());
						ExportEngine::Initialize();
						ExportEngine::ExportBitmap(absScript, absOutput, exportParams->scriptParams, converted.get(), converter.get(), params);
						continue;
					}
					//
					// Sprite dependency
					//
					if (SpriteGetCfg(depName, &params) && params != nullptr) {
						Log::Info(LogChannel::Build, "[Dep] Sprite '%s': loading source image.", depName.c_str());
						std::string sourcePath = SpriteGetSourcePath(depName);
						auto image = Image::ImageLoad(sourcePath);
						if (!image) {
							Log::Error(LogChannel::Build, "[Dep] Sprite '%s': failed to load source image '%s'.", depName.c_str(), sourcePath.c_str());
							continue;
						}
						auto converter = Converters::GetBitmapConverter(params);
						if (!converter) {
							Log::Error(LogChannel::Build, "[Dep] Sprite '%s': no suitable converter for the given parameters.", depName.c_str());
							continue;
						}
						converter->SetOriginal(image);
						Log::Info(LogChannel::Build, "[Dep] Sprite '%s': converting.", depName.c_str());
						converter->Convert(params);
						auto converted = converter->GetConverted(params);
						if (!converted) {
							Log::Error(LogChannel::Build, "[Dep] Sprite '%s': conversion produced no output.", depName.c_str());
							continue;
						}
						SpriteExtractionParams* spriteParams = nullptr;
						SpriteGetSpriteParams(depName, &spriteParams);
						auto spriteExtractor = converter->GetSpriteExtractor();
						if (!spriteExtractor) {
							Log::Error(LogChannel::Build, "[Dep] Sprite '%s': failed to allocate sprite extractor.", depName.c_str());
							continue;
						}
						if (spriteParams) {
							Log::Info(LogChannel::Build, "[Dep] Sprite '%s': extracting sprites.", depName.c_str());
							spriteExtractor->Extract(converted, spriteParams);
						}
						ExportParams* exportParams = nullptr;
						SpriteGetExportParams(depName, &exportParams);
						if (!exportParams || exportParams->scriptPath.empty()) {
							Log::Warning(LogChannel::Build, "[Dep] Sprite '%s': no export script defined, skipping export.", depName.c_str());
							continue;
						}
						std::string absScript = ExpandPath(exportParams->scriptPath);
						std::string absOutput = ExpandPath(exportParams->outputName);
						Log::Info(LogChannel::Build, "[Dep] Sprite '%s': exporting to '%s'.", depName.c_str(), absOutput.c_str());
						ExportEngine::Initialize();
						ExportEngine::ExportSprites(absScript, absOutput, exportParams->scriptParams, converter.get(), params, spriteExtractor.get(), spriteParams);
						continue;
					}
					//
					// Tileset dependency
					//
					if (TilesetGetCfg(depName, &params) && params != nullptr) {
						Log::Info(LogChannel::Build, "[Dep] Tileset '%s': loading source image.", depName.c_str());
						std::string sourcePath = TilesetGetSourcePath(depName);
						auto image = Image::ImageLoad(sourcePath);
						if (!image) {
							Log::Error(LogChannel::Build, "[Dep] Tileset '%s': failed to load source image '%s'.", depName.c_str(), sourcePath.c_str());
							continue;
						}
						auto converter = Converters::GetBitmapConverter(params);
						if (!converter) {
							Log::Error(LogChannel::Build, "[Dep] Tileset '%s': no suitable converter for the given parameters.", depName.c_str());
							continue;
						}
						converter->SetOriginal(image);
						Log::Info(LogChannel::Build, "[Dep] Tileset '%s': converting.", depName.c_str());
						converter->Convert(params);
						auto converted = converter->GetConverted(params);
						if (!converted) {
							Log::Error(LogChannel::Build, "[Dep] Tileset '%s': conversion produced no output.", depName.c_str());
							continue;
						}
						TileExtractionParams* tileParams = nullptr;
						TilesetGetTileParams(depName, &tileParams);
						auto tileExtractor = converter->GetTileExtractor();
						if (!tileExtractor) {
							Log::Error(LogChannel::Build, "[Dep] Tileset '%s': failed to allocate tile extractor.", depName.c_str());
							continue;
						}
						if (tileParams) {
							Log::Info(LogChannel::Build, "[Dep] Tileset '%s': extracting tiles.", depName.c_str());
							tileExtractor->Extract(converted, tileParams);
						}
						ExportParams* exportParams = nullptr;
						TilesetGetExportParams(depName, &exportParams);
						if (!exportParams || exportParams->scriptPath.empty()) {
							Log::Warning(LogChannel::Build, "[Dep] Tileset '%s': no export script defined, skipping export.", depName.c_str());
							continue;
						}
						std::string absScript = ExpandPath(exportParams->scriptPath);
						std::string absOutput = ExpandPath(exportParams->outputName);
						Log::Info(LogChannel::Build, "[Dep] Tileset '%s': exporting to '%s'.", depName.c_str(), absOutput.c_str());
						ExportEngine::Initialize();
						ExportEngine::ExportTileset(absScript, absOutput, exportParams->scriptParams, converter.get(), params, tileExtractor.get(), tileParams);
						continue;
					}
					//
					// Map dependency: grab params and call the exporter directly — no image loading needed
					//
					{
						MapParams* mapParams = nullptr;
						if (MapGetParams(depName, &mapParams) && mapParams != nullptr) {
							ExportParams* exportParams = nullptr;
							MapGetExportParams(depName, &exportParams);
							if (!exportParams || exportParams->scriptPath.empty()) {
								Log::Warning(LogChannel::Build, "[Dep] Map '%s': no export script defined, skipping export.", depName.c_str());
								continue;
							}
							std::string absScript = ExpandPath(exportParams->scriptPath);
							std::string absOutput = ExpandPath(exportParams->outputName);
							Log::Info(LogChannel::Build, "[Dep] Map '%s': exporting to '%s'.", depName.c_str(), absOutput.c_str());
							ExportEngine::Initialize();
							ExportEngine::ExportMap(absScript, absOutput, exportParams->scriptParams, mapParams);
							continue;
						}
					}
					//
					// Palette dependency: solve and validate palette assignments — no export needed
					//
					{
						PaletteParams* paletteParams = nullptr;
						if (PaletteGetParams(depName, &paletteParams) && paletteParams != nullptr) {
							Log::Info(LogChannel::Build, "[Dep] Palette '%s': solving.", depName.c_str());
								std::string projectFolder = std::filesystem::path(currentProjectPath).parent_path().string();
								PaletteSolution solution = PaletteSolver::Solve(paletteParams, projectFolder);
								if (!solution.valid) {
									Log::Error(LogChannel::Build, "[Dep] Palette '%s': no valid solution found.", depName.c_str());
									continue;
								}
								Log::Info(LogChannel::Build, "[Dep] Palette '%s': solution found, validating.", depName.c_str());
								PaletteSolver::Validate(solution, paletteParams);
								continue;
						}
					}

				}
				return true;
			}
		}
		return false;
	}
	//
	// Add an explicitly-created virtual folder to the Build section
	//
	bool Project::FolderAdd(const std::string& folderPath) {
		if (!isProjectOpen || folderPath.empty())
			return false;
		for (const auto& f : currentProject.buildFolders) {
			if (f == folderPath)
				return false;
		}
		currentProject.buildFolders.push_back(folderPath);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a virtual folder and strip its prefix from all contained build items
	//
	bool Project::FolderRemove(const std::string& folderPath) {
		if (!isProjectOpen || folderPath.empty())
			return false;
		//
		// Remove from the folders list
		//
		bool found = false;
		for (auto it = currentProject.buildFolders.begin(); it != currentProject.buildFolders.end(); ++it) {
			if (*it == folderPath) {
				currentProject.buildFolders.erase(it);
				found = true;
				break;
			}
		}
		if (!found)
			return false;
		//
		// Strip the folder prefix (folderPath + "/") from all build items whose name starts with it
		//
		std::string prefix = folderPath + "/";
		auto stripPrefix = [&](std::string& name) {
			if (name.rfind(prefix, 0) == 0)
				name = name.substr(prefix.size());
		};
		for (auto& e : currentProject.buildBitmaps)
			stripPrefix(e.name);
		for (auto& e : currentProject.buildTiles)
			stripPrefix(e.name);
		for (auto& e : currentProject.buildSprites)
			stripPrefix(e.name);
		for (auto& e : currentProject.buildMaps)
			stripPrefix(e.name);
		for (auto& e : currentProject.buildBuilds)
			stripPrefix(e.name);
		for (auto& e : currentProject.buildPalettes)
			stripPrefix(e.name);
		//
		// Also remove any child virtual folders (sub-folders of folderPath)
		//
		currentProject.buildFolders.erase(
			std::remove_if(currentProject.buildFolders.begin(), currentProject.buildFolders.end(), [&](const std::string& f) { return f.rfind(prefix, 0) == 0; }),
			currentProject.buildFolders.end());
		isProjectModified = true;
		return true;
	}
	//
	// Return true if the virtual folder exists in the Build section
	//
	bool Project::FolderExists(const std::string& folderPath) {
		if (!isProjectOpen)
			return false;
		for (const auto& f : currentProject.buildFolders) {
			if (f == folderPath)
				return true;
		}
		return false;
	}
	//
	// Return all explicitly-created virtual folder paths
	//
	std::vector<std::string> Project::GetFolders() {
		if (!isProjectOpen)
			return {};
		return currentProject.buildFolders;
	}
	//
	// Get the name of the currently selected build item
	//
	std::string Project::GetSelectedBuildItem() {
		if (!isProjectOpen)
			return {};
		return currentProject.selectedBuildItem;
	}
	//
	// Set the name of the currently selected build item and mark the project as modified
	//
	void Project::SetSelectedBuildItem(const std::string& name) {
		if (!isProjectOpen)
			return;
		if (currentProject.selectedBuildItem == name)
			return;
		currentProject.selectedBuildItem = name;
		isProjectModified = true;
	}
	//
	// Get parameters for a palette build item
	//
	bool Project::PaletteGetParams(const std::string& name, PaletteParams** params) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildPalettes) {
			if (entry.name == name) {
				if (params != nullptr)
					*params = &entry.paletteParams;
				return true;
			}
		}
		return false;
	}
	//
	// Add a palette build item to the current project
	//
	bool Project::PaletteAdd(const std::string& name) {
		if (!isProjectOpen)
			return false;
		for (const auto& entry : currentProject.buildPalettes) {
			if (entry.name == name)
				return false;
		}
		ProjectBuildPaletteEntry entry;
		entry.name = name;
		currentProject.buildPalettes.push_back(entry);
		isProjectModified = true;
		return true;
	}
	//
	// Remove a palette build item from the current project
	//
	bool Project::PaletteRemove(const std::string& name) {
		if (!isProjectOpen)
			return false;
		for (auto it = currentProject.buildPalettes.begin(); it != currentProject.buildPalettes.end(); ++it) {
			if (it->name == name) {
				currentProject.buildPalettes.erase(it);
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Rename a palette build item in the current project
	//
	bool Project::PaletteRename(const std::string& oldName, const std::string& newName) {
		if (!isProjectOpen)
			return false;
		for (const auto& entry : currentProject.buildPalettes) {
			if (entry.name == newName)
				return false;
		}
		for (auto& entry : currentProject.buildPalettes) {
			if (entry.name == oldName) {
				entry.name = newName;
				isProjectModified = true;
				return true;
			}
		}
		return false;
	}
	//
	// Get export parameters for a palette build item
	//
	bool Project::PaletteGetExportParams(const std::string& name, ExportParams** exportParams) {
		if (!isProjectOpen)
			return false;
		for (auto& entry : currentProject.buildPalettes) {
			if (entry.name == name) {
				if (exportParams != nullptr)
					*exportParams = &entry.exportParams;
				return true;
			}
		}
		return false;
	}

} 