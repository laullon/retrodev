//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#include "imgui.filedialog.h"

#include <fstream>
#include <algorithm>
#include <chrono>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <lmcons.h>
#else
#include <cstdlib>
#endif

#define GUI_ELEMENT_SIZE std::max(GImGui->FontSize + 10.f, 24.f)
#define PI 3.141592f

namespace ImGui {

	//
	// Internal ImGui widget helpers used exclusively by FileDialog.
	//

	// Collapsible folder row with an expand arrow, a font icon and a label.
	// Clicking the arrow toggles expansion; clicking the label sets 'clicked'.
	// Double-clicking the label also toggles expansion.
	// Returns true while the node is open (children must be rendered and TreePop called by the caller).
	bool FolderNode(const char* label, const char* fontIcon, const char* fontIconOpen, bool& clicked) {
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		clicked = false;

		// Retrieve persistent open/closed state from window storage
		ImU32 id = window->GetID(label);
		int opened = window->StateStorage.GetInt(id, 0);
		ImVec2 pos = window->DC.CursorPos;
		const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= pos.x && g.IO.MousePos.x < pos.x + g.FontSize);
		if (ImGui::InvisibleButton(label, ImVec2(-FLT_MIN, g.FontSize + g.Style.FramePadding.y * 2))) {
			if (is_mouse_x_over_arrow) {
				// Toggle expand/collapse when clicking the arrow area
				int* p_opened = window->StateStorage.GetIntRef(id, 0);
				opened = *p_opened = (*p_opened == 0) ? 1 : 0;
			} else {
				// Navigate to this directory when clicking the label
				clicked = true;
			}
		}
		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();
		bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
		if (doubleClick && hovered) {
			// Double-click on label toggles expansion without triggering navigation
			int* p_opened = window->StateStorage.GetIntRef(id, 0);
			opened = *p_opened = (*p_opened == 0) ? 1 : 0;
			clicked = false;
		}
		if (hovered || active)
			window->DrawList->AddRectFilled(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max,
											ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered]));

		// Render arrow, icon and label text
		const char* currentIcon = opened ? fontIconOpen : fontIcon;
		float icon_posX = pos.x + g.FontSize + g.Style.FramePadding.y;
		ImVec2 iconTextSize = ImGui::CalcTextSize(currentIcon);
		float text_posX = icon_posX + iconTextSize.x + g.Style.FramePadding.y;
		ImGui::RenderArrow(window->DrawList, ImVec2(pos.x, pos.y + g.Style.FramePadding.y),
						   ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[((hovered && is_mouse_x_over_arrow) || opened) ? ImGuiCol_Text : ImGuiCol_TextDisabled]),
						   opened ? ImGuiDir_Down : ImGuiDir_Right);
		ImGui::RenderText(ImVec2(icon_posX, pos.y + g.Style.FramePadding.y), currentIcon);
		ImGui::RenderText(ImVec2(text_posX, pos.y + g.Style.FramePadding.y), label);
		if (opened)
			ImGui::TreePush(label);
		return opened != 0;
	}
	// Non-expandable file row with a font icon and a label. Returns true when clicked.
	bool FileNode(const char* label, const char* fontIcon) {
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		// ImU32 id = window->GetID(label);
		ImVec2 pos = window->DC.CursorPos;
		bool ret = ImGui::InvisibleButton(label, ImVec2(-FLT_MIN, g.FontSize + g.Style.FramePadding.y * 2));

		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();
		if (hovered || active)
			window->DrawList->AddRectFilled(g.LastItemData.Rect.Min, g.LastItemData.Rect.Max,
											ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered]));

		// Render icon then label side by side
		ImVec2 iconTextSize = ImGui::CalcTextSize(fontIcon);
		ImGui::RenderText(ImVec2(pos.x, pos.y + g.Style.FramePadding.y), fontIcon);
		ImGui::RenderText(ImVec2(pos.x + iconTextSize.x + g.Style.FramePadding.y, pos.y + g.Style.FramePadding.y), label);

		return ret;
	}
	// Breadcrumb path bar that renders each path component as a clickable button.
	// Clicking a component navigates directly to that ancestor directory.
	// Clicking the background of the bar switches it into a text-input mode so
	// the user can type an arbitrary path. Returns true when the path changes.
	// state bit layout: bit0 = text-input mode active, bit1 = hovered, bit2 = keyboard focus consumed.
	bool PathBox(const char* label, std::filesystem::path& path, char* pathBuffer, ImVec2 size_arg) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		bool ret = false;
		const ImGuiID id = window->GetID(label);
		int* state = window->StateStorage.GetIntRef(id, 0);

		ImGui::SameLine();

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		ImVec2 pos = window->DC.CursorPos;
		ImVec2 uiPos = ImGui::GetCursorPos();
		ImVec2 size = ImGui::CalcItemSize(size_arg, 200, GUI_ELEMENT_SIZE);
		const ImRect bb(pos, pos + size);

		// Breadcrumb button mode
		if (!(*state & 0b001)) {
			ImGui::PushClipRect(bb.Min, bb.Max, false);

			// background
			bool hovered = g.IO.MousePos.x >= bb.Min.x && g.IO.MousePos.x <= bb.Max.x && g.IO.MousePos.y >= bb.Min.y && g.IO.MousePos.y <= bb.Max.y;
			bool clicked = hovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left);
			bool anyOtherHC = false; // are any other items hovered or clicked?
			window->DrawList->AddRectFilled(pos, pos + size,
											ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[(*state & 0b10) ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg]));

			// Collect path components, skipping bare separator entries
			std::vector<std::string> btnList;
			float totalWidth = 0.0f;
			for (auto comp : path) {
				std::string section = comp.string();
				if (section.size() == 1 && (section[0] == '\\' || section[0] == '/'))
					continue;

				totalWidth += ImGui::CalcTextSize(section.c_str()).x + style.FramePadding.x * 2.0f + GUI_ELEMENT_SIZE;
				btnList.push_back(section);
			}
			totalWidth -= GUI_ELEMENT_SIZE;

			// Render one button per component; trim leading components when there is not enough horizontal space
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, ImGui::GetStyle().ItemSpacing.y));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
			bool isFirstElement = true;
			for (size_t i = 0; i < btnList.size(); i++) {
				if (totalWidth > size.x - 30 && i != btnList.size() - 1) { // trim some buttons if there's not enough space
					float elSize = ImGui::CalcTextSize(btnList[i].c_str()).x + style.FramePadding.x * 2.0f + GUI_ELEMENT_SIZE;
					totalWidth -= elSize;
					continue;
				}

				ImGui::PushID(static_cast<int>(i));
				if (!isFirstElement) {
					ImGui::ArrowButtonEx("##dir_dropdown", ImGuiDir_Right, ImVec2(GUI_ELEMENT_SIZE, GUI_ELEMENT_SIZE));
					anyOtherHC |= ImGui::IsItemHovered() || ImGui::IsItemClicked();
					ImGui::SameLine();
				}
				if (ImGui::Button(btnList[i].c_str(), ImVec2(0, GUI_ELEMENT_SIZE))) {
#ifdef _WIN32
					std::string newPath = "";
#else
					std::string newPath = "/";
#endif
					for (size_t j = 0; j <= i; j++) {
						newPath += btnList[j];
#ifdef _WIN32
						if (j != i)
							newPath += "\\";
#else
						if (j != i)
							newPath += "/";
#endif
					}
					path = std::filesystem::path(newPath);
					ret = true;
				}
				anyOtherHC |= ImGui::IsItemHovered() || ImGui::IsItemClicked();
				ImGui::SameLine();
				ImGui::PopID();

				isFirstElement = false;
			}
			ImGui::PopStyleVar(2);

			// click state
			if (!anyOtherHC && clicked) {
				strcpy(pathBuffer, path.string().c_str());
				*state |= 0b001;
				*state &= 0b011; // remove SetKeyboardFocus flag
			} else
				*state &= 0b110;

			// hover state
			if (!anyOtherHC && hovered && !clicked)
				*state |= 0b010;
			else
				*state &= 0b101;

			ImGui::PopClipRect();

			// allocate space
			ImGui::SetCursorPos(uiPos);
			ImGui::ItemSize(size);
		}
		// Text-input mode: user clicked the bar background to type a path manually
		else {
			bool skipActiveCheck = false;
			if (!(*state & 0b100)) {
				// Focus the input on the first frame it becomes active
				skipActiveCheck = true;
				ImGui::SetKeyboardFocusHere();
				if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
					*state |= 0b100;
			}
			if (ImGui::InputTextEx("##pathbox_input", "", pathBuffer, 1024, size_arg, ImGuiInputTextFlags_EnterReturnsTrue)) {
				// Only accept the typed path if it actually exists on disk
				std::string tempStr(pathBuffer);
				if (std::filesystem::exists(tempStr))
					path = std::filesystem::path(tempStr);
				ret = true;
			}
			// Return to breadcrumb mode when the input loses focus
			if (!skipActiveCheck && !ImGui::IsItemActive())
				*state &= 0b010;
		}

		return ret;
	}
	// Star-shaped toggle button used to bookmark the current directory.
	// When isFavorite is true or the button is hovered/active, the star is filled.
	// The star is always outlined. Returns true when clicked.
	bool FavoriteButton(const char* label, bool isFavorite) {
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		ImVec2 pos = window->DC.CursorPos;
		bool ret = ImGui::InvisibleButton(label, ImVec2(GUI_ELEMENT_SIZE, GUI_ELEMENT_SIZE));

		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();

		float size = g.LastItemData.Rect.Max.x - g.LastItemData.Rect.Min.x;

		// 5-point star geometry
		int numPoints = 5;
		float innerRadius = size / 4;
		float outerRadius = size / 2;
		float angle = PI / numPoints;
		ImVec2 center = ImVec2(pos.x + size / 2, pos.y + size / 2);

		// Fill the star when it is a favourite or the user is interacting with it
		if (isFavorite || hovered || active) {
			ImU32 fillColor = 0xff00ffff; // ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);
			if (hovered || active)
				fillColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : ImGuiCol_HeaderHovered]);

			// PathFillConcave is unavailable, so fill the concave star in two passes:
			// first the convex inner pentagon, then each outer triangle individually
			window->DrawList->PathClear();
			for (int i = 1; i < numPoints * 2; i += 2)
				window->DrawList->PathLineTo(ImVec2(center.x + innerRadius * sin(i * angle), center.y - innerRadius * cos(i * angle)));
			window->DrawList->PathFillConvex(fillColor);

			// Outer point triangles
			for (int i = 0; i < numPoints; i++) {
				window->DrawList->PathClear();

				int pIndex = i * 2;
				window->DrawList->PathLineTo(ImVec2(center.x + outerRadius * sin(pIndex * angle), center.y - outerRadius * cos(pIndex * angle)));
				window->DrawList->PathLineTo(ImVec2(center.x + innerRadius * sin((pIndex + 1) * angle), center.y - innerRadius * cos((pIndex + 1) * angle)));
				window->DrawList->PathLineTo(ImVec2(center.x + innerRadius * sin((pIndex - 1) * angle), center.y - innerRadius * cos((pIndex - 1) * angle)));

				window->DrawList->PathFillConvex(fillColor);
			}
		}

		// Always draw the star outline
		window->DrawList->PathClear();
		for (int i = 0; i < numPoints * 2; i++) {
			float radius = i & 1 ? innerRadius : outerRadius;
			window->DrawList->PathLineTo(ImVec2(center.x + radius * sin(i * angle), center.y - radius * cos(i * angle)));
		}
		window->DrawList->PathStroke(ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), true, 2.0f);

		return ret;
	}
	// Thumbnail icon widget used in the icon-view grid.
	// Renders a large font icon centred in the upper area and the filename wrapped below it.
	// Automatically calls SameLine() when there is still horizontal space for another icon.
	// Returns true on single click or double-click while hovered.
	bool FileIcon(const char* label, bool isSelected, const char* fontIcon, ImVec2 size) {
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;

		float windowSpace = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
		ImVec2 pos = window->DC.CursorPos;
		bool ret = false;

		if (ImGui::InvisibleButton(label, size))
			ret = true;

		bool hovered = ImGui::IsItemHovered();
		bool active = ImGui::IsItemActive();
		bool doubleClick = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
		if (doubleClick && hovered)
			ret = true;

		// Upper portion is icon area; lower two lines are reserved for the filename
		float iconAreaHeight = size.y - g.FontSize * 2;
		ImVec2 textSize = ImGui::CalcTextSize(label, 0, true, size.x);

		if (hovered || active || isSelected)
			window->DrawList->AddRectFilled(
				g.LastItemData.Rect.Min, g.LastItemData.Rect.Max,
				ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[active ? ImGuiCol_HeaderActive : (isSelected ? ImGuiCol_Header : ImGuiCol_HeaderHovered)]));

		// Scale the font icon to fill 60 % of the icon area, clamped to at least the base font size
		float iconFontSize = iconAreaHeight * 0.6f;
		if (iconFontSize < g.FontSize)
			iconFontSize = g.FontSize;
		ImVec2 iconTextSize = ImGui::CalcTextSize(fontIcon);
		float iconScale = iconFontSize / g.FontSize;
		float scaledIconW = iconTextSize.x * iconScale;
		float scaledIconH = iconTextSize.y * iconScale;
		float iconPosX = pos.x + (size.x - scaledIconW) / 2.0f;
		float iconPosY = pos.y + (iconAreaHeight - scaledIconH) / 2.0f;
		window->DrawList->AddText(g.Font, iconFontSize, ImVec2(iconPosX, iconPosY), ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), fontIcon);

		// Render the filename centred and word-wrapped below the icon
		window->DrawList->AddText(g.Font, g.FontSize, ImVec2(pos.x + (size.x - textSize.x) / 2.0f, pos.y + iconAreaHeight),
								  ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]), label, 0, size.x);

		// Wrap to the next row only when this icon would overflow the available width
		float lastButtomPos = ImGui::GetItemRectMax().x;
		float thisButtonPos = lastButtomPos + style.ItemSpacing.x + size.x;
		if (thisButtonPos < windowSpace)
			ImGui::SameLine();

		return ret;
	}

	FileDialog::FileData::FileData(const std::filesystem::path& path) {
		std::error_code ec;
		Path = path;
		IsDirectory = std::filesystem::is_directory(path, ec);
		Size = std::filesystem::file_size(path, ec);

		// Convert the filesystem clock time-point to a system_clock time_t for display
		auto ftime = std::filesystem::last_write_time(path, ec);
		if (!ec) {
			auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
			DateModified = std::chrono::system_clock::to_time_t(sctp);
		} else {
			DateModified = 0;
		}
	}

	FileDialog::FileDialog() {
		m_isOpen = false;
		m_type = Type::File;
		m_calledOpenPopup = false;
		m_sortColumn = 0;
		m_sortDirection = ImGuiSortDirection_Ascending;
		m_filterSelection = 0;
		m_inputTextbox[0] = 0;
		m_pathBuffer[0] = 0;
		m_searchBuffer[0] = 0;
		m_newEntryBuffer[0] = 0;
		m_selectedFileItem = -1;
		m_zoom = 1.0f;
		ShowHiddenFiles = false;
		m_treeSyncRequired = false;

		// Start in the process working directory
		m_setDirectory(std::filesystem::current_path(), false);

		// The Quick Access node is pre-populated and shared across all platforms
		FileTreeNode* quickAccess = new FileTreeNode("Quick Access");
		quickAccess->Read = true;
		m_treeCache.push_back(quickAccess);

#ifdef _WIN32
		char usernameA[UNLEN + 1] = {0};
		DWORD username_len = UNLEN + 1;
		GetUserNameA(usernameA, &username_len);

		std::string userPath = "C:\\Users\\" + std::string(usernameA) + "\\";

		// Quick Access bookmarks for the current user
		quickAccess->Children.push_back(new FileTreeNode(userPath + "Desktop"));
		quickAccess->Children.push_back(new FileTreeNode(userPath + "Documents"));
		quickAccess->Children.push_back(new FileTreeNode(userPath + "Downloads"));
		quickAccess->Children.push_back(new FileTreeNode(userPath + "Pictures"));

		// OneDrive root (lazy-loaded on first expand)
		FileTreeNode* oneDrive = new FileTreeNode(userPath + "OneDrive");
		m_treeCache.push_back(oneDrive);

		// This PC: standard shell folders + all detected logical drives
		FileTreeNode* thisPC = new FileTreeNode("This PC");
		thisPC->Read = true;
		if (std::filesystem::exists(userPath + "3D Objects"))
			thisPC->Children.push_back(new FileTreeNode(userPath + "3D Objects"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Desktop"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Documents"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Downloads"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Music"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Pictures"));
		thisPC->Children.push_back(new FileTreeNode(userPath + "Videos"));
		DWORD d = GetLogicalDrives();
		for (int i = 0; i < 26; i++)
			if (d & (1 << i))
				thisPC->Children.push_back(new FileTreeNode(std::string(1, 'A' + i) + ":"));
		m_treeCache.push_back(thisPC);
#else
		std::error_code ec;

		// Quick Access: home directory and common subdirectories (only those that exist)
		const char* homeEnv = std::getenv("HOME");
		if (homeEnv) {
			std::string homePath(homeEnv);

			if (std::filesystem::exists(homePath, ec))
				quickAccess->Children.push_back(new FileTreeNode(homePath));
			if (std::filesystem::exists(homePath + "/Desktop", ec))
				quickAccess->Children.push_back(new FileTreeNode(homePath + "/Desktop"));
			if (std::filesystem::exists(homePath + "/Documents", ec))
				quickAccess->Children.push_back(new FileTreeNode(homePath + "/Documents"));
			if (std::filesystem::exists(homePath + "/Downloads", ec))
				quickAccess->Children.push_back(new FileTreeNode(homePath + "/Downloads"));
			if (std::filesystem::exists(homePath + "/Pictures", ec))
				quickAccess->Children.push_back(new FileTreeNode(homePath + "/Pictures"));
		}

		// This PC: top-level filesystem entries under root
		FileTreeNode* thisPC = new FileTreeNode("This PC");
		thisPC->Read = true;
		for (const auto& entry : std::filesystem::directory_iterator("/", ec)) {
			if (std::filesystem::is_directory(entry, ec))
				thisPC->Children.push_back(new FileTreeNode(entry.path().string()));
		}
		m_treeCache.push_back(thisPC);
#endif
	}
	FileDialog::~FileDialog() {
		for (auto fn : m_treeCache)
			m_clearTree(fn);
		m_treeCache.clear();
	}
	bool FileDialog::Save(const std::string& key, const std::string& title, const std::string& filter, const std::string& startingDir) {
		if (!m_currentKey.empty())
			return false;

		m_currentKey = key;
		m_currentTitle = title + "###" + key;
		m_isOpen = true;
		m_calledOpenPopup = false;
		m_result.clear();
		m_inputTextbox[0] = 0;
		m_selections.clear();
		m_selectedFileItem = -1;
		m_isMultiselect = false;
		m_type = Type::Save;

		m_parseFilter(filter);
		if (!startingDir.empty())
			m_setDirectory(std::filesystem::path(startingDir), false);
		else
			m_setDirectory(m_currentDirectory, false); // refresh contents

		return true;
	}
	bool FileDialog::Open(const std::string& key, const std::string& title, const std::string& filter, bool isMultiselect, const std::string& startingDir) {
		if (!m_currentKey.empty())
			return false;

		m_currentKey = key;
		m_currentTitle = title + "###" + key;
		m_isOpen = true;
		m_calledOpenPopup = false;
		m_result.clear();
		m_inputTextbox[0] = 0;
		m_selections.clear();
		m_selectedFileItem = -1;
		m_isMultiselect = isMultiselect;
		m_type = filter.empty() ? Type::Directory : Type::File;

		m_parseFilter(filter);
		if (!startingDir.empty())
			m_setDirectory(std::filesystem::path(startingDir), false);
		else
			m_setDirectory(m_currentDirectory, false); // refresh contents

		return true;
	}
	bool FileDialog::IsDone(const std::string& key) {
		bool isMe = m_currentKey == key;

		if (isMe && m_isOpen) {
			if (!m_calledOpenPopup) {
				ImGui::SetNextWindowSize(ImVec2(GUI_ELEMENT_SIZE * 40, GUI_ELEMENT_SIZE * 30), ImGuiCond_FirstUseEver);
				ImGui::OpenPopup(m_currentTitle.c_str());
				m_calledOpenPopup = true;
			}

			if (ImGui::BeginPopupModal(m_currentTitle.c_str(), &m_isOpen, ImGuiWindowFlags_NoScrollbar)) {
				m_renderFileDialog();
				ImGui::EndPopup();
			} else
				m_isOpen = false;
		}

		return isMe && !m_isOpen;
	}
	void FileDialog::Close() {
		m_currentKey.clear();
		m_backHistory = std::stack<std::filesystem::path>();
		m_forwardHistory = std::stack<std::filesystem::path>();

		// clear the tree
		for (auto fn : m_treeCache) {
			for (auto item : fn->Children) {
				for (auto ch : item->Children)
					m_clearTree(ch);
				item->Children.clear();
				item->Read = false;
			}
		}
	}

	void FileDialog::RemoveFavorite(const std::string& path) {
		auto itr = std::find(m_favorites.begin(), m_favorites.end(), m_currentDirectory.string());

		if (itr != m_favorites.end())
			m_favorites.erase(itr);

		// remove from sidebar
		for (auto& p : m_treeCache)
			if (p->Path == "Quick Access") {
				for (size_t i = 0; i < p->Children.size(); i++)
					if (p->Children[i]->Path == path) {
						p->Children.erase(p->Children.begin() + i);
						break;
					}
				break;
			}
	}
	void FileDialog::AddFavorite(const std::string& path) {
		if (std::count(m_favorites.begin(), m_favorites.end(), path) > 0)
			return;

		if (!std::filesystem::exists(std::filesystem::path(path)))
			return;

		m_favorites.push_back(path);

		// add to sidebar
		for (auto& p : m_treeCache)
			if (p->Path == "Quick Access") {
				p->Children.push_back(new FileTreeNode(path));
				break;
			}
	}

	// Adds or removes 'path' from the current selection.
	// Without Ctrl: replaces the selection with only this path.
	// With Ctrl (and multiselect enabled): toggles the path in the selection.
	// Always synchronises m_inputTextbox with the resulting selection.
	void FileDialog::m_select(const std::filesystem::path& path, bool isCtrlDown) {
		bool multiselect = isCtrlDown && m_isMultiselect;

		if (!multiselect) {
			m_selections.clear();
			m_selections.push_back(path);
		} else {
			// Toggle: deselect if already selected, otherwise append
			auto it = std::find(m_selections.begin(), m_selections.end(), path);
			if (it != m_selections.end())
				m_selections.erase(it);
			else
				m_selections.push_back(path);
		}

		if (m_selections.size() == 1) {
			// Single selection: show just the filename (fall back to full path for drives)
			std::string filename = m_selections[0].filename().string();
			if (filename.size() == 0)
				filename = m_selections[0].string(); // drive

			strcpy(m_inputTextbox, filename.c_str());
		} else {
			// Multi-selection: show all filenames as a quoted, comma-separated list
			std::string textboxVal = "";
			for (const auto& sel : m_selections) {
				std::string filename = sel.filename().string();
				if (filename.size() == 0)
					filename = sel.string();

				textboxVal += "\"" + filename + "\", ";
			}
			strcpy(m_inputTextbox, textboxVal.substr(0, textboxVal.size() - 2).c_str());
		}
	}

	// Commits the current selection/filename into m_result and closes the dialog.
	// For File/Directory modes, all resolved paths must exist on disk; returns false
	// and clears m_result if any path is missing.
	// For Save mode, appends the active filter's first extension when the filename
	// has no extension.
	bool FileDialog::m_finalize(const std::string& filename) {
		// Directory mode accepts an empty filename (the current directory is the result)
		bool hasResult = (!filename.empty() && m_type != Type::Directory) || m_type == Type::Directory;

		if (hasResult) {
			if (!m_isMultiselect || m_selections.size() <= 1) {
				// Single result: resolve relative paths against the current directory
				std::filesystem::path path = std::filesystem::path(filename);
				if (path.is_absolute())
					m_result.push_back(path);
				else
					m_result.push_back(m_currentDirectory / path);
				if (m_type == Type::Directory || m_type == Type::File) {
					if (!std::filesystem::exists(m_result.back())) {
						m_result.clear();
						return false;
					}
				}
			} else {
				// Multi-result: resolve each selection and verify existence
				for (const auto& sel : m_selections) {
					if (sel.is_absolute())
						m_result.push_back(sel);
					else
						m_result.push_back(m_currentDirectory / sel);
					if (m_type == Type::Directory || m_type == Type::File) {
						if (!std::filesystem::exists(m_result.back())) {
							m_result.clear();
							return false;
						}
					}
				}
			}

			if (m_type == Type::Save) {
				// Auto-append the first extension from the active filter when none is present
				if (m_filterSelection < m_filterExtensions.size() && m_filterExtensions[m_filterSelection].size() > 0) {
					if (!m_result.back().has_extension()) {
						std::string extAdd = m_filterExtensions[m_filterSelection][0];
						m_result.back().replace_extension(extAdd);
					}
				}
			}
		}

		m_isOpen = false;

		return true;
	}
	// Parses a filter string of the form "Label{.ext1,.ext2},Label2{.ext3}" into
	// m_filter (null-separated ImGui Combo string) and m_filterExtensions (per-entry
	// extension lists). The special label ".*" expands to "All Files (*.*)".
	void FileDialog::m_parseFilter(const std::string& filter) {
		m_filter = "";
		m_filterExtensions.clear();
		m_filterSelection = 0;

		if (filter.empty())
			return;

		std::vector<std::string> exts;

		size_t lastSplit = 0, lastExt = 0;
		bool inExtList = false;
		for (size_t i = 0; i < filter.size(); i++) {
			if (filter[i] == ',') {
				if (!inExtList)
					lastSplit = i + 1;
				else {
					exts.push_back(filter.substr(lastExt, i - lastExt));
					lastExt = i + 1;
				}
			} else if (filter[i] == '{') {
				std::string filterName = filter.substr(lastSplit, i - lastSplit);
				if (filterName == ".*") {
					m_filter += std::string(std::string("All Files (*.*)\0").c_str(), 16);
					m_filterExtensions.push_back(std::vector<std::string>());
				} else
					m_filter += std::string((filterName + "\0").c_str(), filterName.size() + 1);
				inExtList = true;
				lastExt = i + 1;
			} else if (filter[i] == '}') {
				exts.push_back(filter.substr(lastExt, i - lastExt));
				m_filterExtensions.push_back(exts);
				exts.clear();

				inExtList = false;
			}
		}
		if (lastSplit != 0) {
			std::string filterName = filter.substr(lastSplit);
			if (filterName == ".*") {
				m_filter += std::string(std::string("All Files (*.*)\0").c_str(), 16);
				m_filterExtensions.push_back(std::vector<std::string>());
			} else
				m_filter += std::string((filterName + "\0").c_str(), filterName.size() + 1);
		}
	}

	const std::string& FileDialog::m_getIconFont(const std::filesystem::path& path) {
		std::error_code ec;
		if (std::filesystem::is_directory(path, ec))
			return FolderIcon;

		if (path.has_extension()) {
			std::string ext = path.extension().string();
			auto it = FileTypeIcons.find(ext);
			if (it != FileTypeIcons.end())
				return it->second;
		}

		return DefaultFileIcon;
	}
	bool FileDialog::m_isHidden(const std::filesystem::path& path) {
#ifdef _WIN32
		DWORD attrs = GetFileAttributesA(path.string().c_str());
		return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_HIDDEN);
#else
		std::string name = path.filename().string();
		return !name.empty() && name[0] == '.';
#endif
	}
	void FileDialog::m_clearTree(FileTreeNode* node) {
		if (node == nullptr)
			return;

		for (auto n : node->Children)
			m_clearTree(n);

		delete node;
		node = nullptr;
	}
	void FileDialog::m_setDirectory(const std::filesystem::path& p, bool addHistory) {
		bool isSameDir = m_currentDirectory == p;

		if (addHistory && !isSameDir)
			m_backHistory.push(m_currentDirectory);

		m_currentDirectory = p;
#ifdef _WIN32
		// drives don't work well without the backslash symbol
		if (p.string().size() == 2 && p.string()[1] == ':')
			m_currentDirectory = std::filesystem::path(p.string() + "\\");
#endif

		m_content.clear(); // p == "" after this line, due to reference
		m_selectedFileItem = -1;

		if (m_type == Type::Directory || m_type == Type::File)
			m_inputTextbox[0] = 0;
		m_selections.clear();

		if (!isSameDir) {
			m_searchBuffer[0] = 0;
		}

		// Virtual nodes: populate from the in-memory tree cache rather than the filesystem
		if (p.string() == "Quick Access") {
			for (auto& node : m_treeCache) {
				if (node->Path == p)
					for (auto& c : node->Children)
						m_content.push_back(FileData(c->Path));
			}
		} else if (p.string() == "This PC") {
			for (auto& node : m_treeCache) {
				if (node->Path == p)
					for (auto& c : node->Children)
						m_content.push_back(FileData(c->Path));
			}
		} else {
			// Real directory: iterate and apply visibility/type/search/extension filters
			std::error_code ec;
			if (std::filesystem::exists(m_currentDirectory, ec))
				for (const auto& entry : std::filesystem::directory_iterator(m_currentDirectory, ec)) {
					FileData info(entry.path());

					// skip hidden files/folders
					if (!ShowHiddenFiles && m_isHidden(entry.path()))
						continue;

					// skip files when Directory dialog
					if (!info.IsDirectory && m_type == Type::Directory)
						continue;

					// check if filename matches search query (case-insensitive)
					if (m_searchBuffer[0]) {
						std::string filename = info.Path.string();

						std::string filenameSearch = filename;
						std::string query(m_searchBuffer);
						std::transform(filenameSearch.begin(), filenameSearch.end(), filenameSearch.begin(), ::tolower);
						std::transform(query.begin(), query.end(), query.begin(), ::tolower);

						if (filenameSearch.find(query, 0) == std::string::npos)
							continue;
					}

					// check if extension matches the active filter (empty extension list means all files pass)
					if (!info.IsDirectory && m_type != Type::Directory) {
						if (m_filterSelection < m_filterExtensions.size()) {
							const auto& exts = m_filterExtensions[m_filterSelection];
							if (exts.size() > 0) {
								std::string extension = info.Path.extension().string();

								// extension not found? skip
								if (std::count(exts.begin(), exts.end(), extension) == 0)
									continue;
							}
						}
					}

					m_content.push_back(info);
				}
		}

		m_sortContent(m_sortColumn, m_sortDirection);
		m_treeSyncRequired = true;
	}
	// Sorts m_content by the given column (0=name, 1=date modified, 2=size) and direction.
	// Directories are always grouped before files; each group is sorted independently.
	void FileDialog::m_sortContent(unsigned int column, unsigned int sortDirection) {
		m_sortColumn = column;
		m_sortDirection = sortDirection;

		// Partition so all directories come before files
		std::partition(m_content.begin(), m_content.end(), [](const FileData& data) { return data.IsDirectory; });

		if (m_content.size() > 0) {
			// Find the boundary between the directory and file halves
			size_t fileIndex = 0;
			for (; fileIndex < m_content.size(); fileIndex++)
				if (!m_content[fileIndex].IsDirectory)
					break;

			// Shared comparator used for both the directory and file sub-ranges
			auto compareFn = [column, sortDirection](const FileData& left, const FileData& right) -> bool {
				// name
				if (column == 0) {
					std::string lName = left.Path.string();
					std::string rName = right.Path.string();

					std::transform(lName.begin(), lName.end(), lName.begin(), ::tolower);
					std::transform(rName.begin(), rName.end(), rName.begin(), ::tolower);

					int comp = lName.compare(rName);

					if (sortDirection == ImGuiSortDirection_Ascending)
						return comp < 0;
					return comp > 0;
				}
				// date
				else if (column == 1) {
					if (sortDirection == ImGuiSortDirection_Ascending)
						return left.DateModified < right.DateModified;
					else
						return left.DateModified > right.DateModified;
				}
				// size
				else if (column == 2) {
					if (sortDirection == ImGuiSortDirection_Ascending)
						return left.Size < right.Size;
					else
						return left.Size > right.Size;
				}

				return false;
			};

			// sort the directories
			std::sort(m_content.begin(), m_content.begin() + fileIndex, compareFn);

			// sort the files
			std::sort(m_content.begin() + fileIndex, m_content.end(), compareFn);
		}
	}

	void FileDialog::m_renderTree(FileTreeNode* node) {
		// directory
		std::error_code ec;
		ImGui::PushID(node);
		bool isClicked = false;
		std::string displayName = node->Path.filename().string();
		if (displayName.size() == 0)
			displayName = node->Path.string();

		// Determine whether this node sits on the path from the root to the current directory;
		// if so it must be forced open so the user can see where they are in the tree.
		bool shouldOpen = false;
		if (m_treeSyncRequired) {
			std::string nodeStr = node->Path.string();
			std::string curStr = m_currentDirectory.string();
			if (nodeStr == "This PC") {
				// For "This PC", check whether any child drive/folder is an ancestor
				for (auto c : node->Children) {
					std::string childStr = c->Path.string();
					if (curStr == childStr || (curStr.size() > childStr.size() && curStr.compare(0, childStr.size(), childStr) == 0 &&
											   (curStr[childStr.size()] == '\\' || curStr[childStr.size()] == '/'))) {
						shouldOpen = true;
						break;
					}
				}
			} else if (nodeStr != "Quick Access") {
				// Open when this node IS the current directory or is an ancestor of it
				shouldOpen = (curStr == nodeStr) || (curStr.size() > nodeStr.size() && curStr.compare(0, nodeStr.size(), nodeStr) == 0 &&
													 (curStr[nodeStr.size()] == '\\' || curStr[nodeStr.size()] == '/'));
			}
		}

		// Eagerly populate children before forcing the node open
		if (shouldOpen && !node->Read) {
			std::filesystem::path iterPath = node->Path;
#ifdef _WIN32
			std::string pathStr = iterPath.string();
			if (pathStr.size() == 2 && pathStr[1] == ':')
				iterPath = std::filesystem::path(pathStr + "\\");
#endif
			if (std::filesystem::exists(iterPath, ec))
				for (const auto& entry : std::filesystem::directory_iterator(iterPath, ec)) {
					if (std::filesystem::is_directory(entry, ec)) {
						if (!ShowHiddenFiles && m_isHidden(entry.path()))
							continue;
						node->Children.push_back(new FileTreeNode(entry.path().string()));
					}
				}
			node->Read = true;
		}

		// Force FolderNode open via StateStorage (SetNextItemOpen doesn't work with custom FolderNode)
		if (shouldOpen && node->Path != m_currentDirectory) {
			ImGuiWindow* window = GImGui->CurrentWindow;
			ImU32 id = window->GetID(displayName.c_str());
			window->StateStorage.SetInt(id, 1);
		}

		if (FolderNode(displayName.c_str(), FolderIcon.c_str(), FolderOpenIcon.c_str(), isClicked)) {
			if (!node->Read) {
				// cache children if it's not already cached
				std::filesystem::path iterPath = node->Path;
#ifdef _WIN32
				// drives need the trailing backslash to iterate from the root
				std::string pathStr = iterPath.string();
				if (pathStr.size() == 2 && pathStr[1] == ':')
					iterPath = std::filesystem::path(pathStr + "\\");
#endif
				if (std::filesystem::exists(iterPath, ec))
					for (const auto& entry : std::filesystem::directory_iterator(iterPath, ec)) {
						if (std::filesystem::is_directory(entry, ec)) {
							if (!ShowHiddenFiles && m_isHidden(entry.path()))
								continue;
							node->Children.push_back(new FileTreeNode(entry.path().string()));
						}
					}
				node->Read = true;
			}

			// display children
			for (auto c : node->Children)
				m_renderTree(c);

			ImGui::TreePop();
		}
		if (m_treeSyncRequired && node->Path == m_currentDirectory) {
			ImGui::SetScrollHereY();
		}
		if (isClicked)
			m_setDirectory(node->Path);
		ImGui::PopID();
	}
	void FileDialog::m_renderContent() {
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			m_selectedFileItem = -1;

		// table view
		if (m_zoom == 1.0f) {
			if (ImGui::BeginTable("##contentTable", 3, /*ImGuiTableFlags_Resizable |*/ ImGuiTableFlags_Sortable, ImVec2(0, -FLT_MIN))) {
				// header
				ImGui::TableSetupColumn("Name##filename", ImGuiTableColumnFlags_WidthStretch, 0.0f - 1.0f, 0);
				ImGui::TableSetupColumn("Date modified##filedate", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0.0f, 1);
				ImGui::TableSetupColumn("Size##filesize", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 0.0f, 2);
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();

				// sort
				if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
					if (sortSpecs->SpecsDirty) {
						sortSpecs->SpecsDirty = false;
						m_sortContent(sortSpecs->Specs->ColumnUserID, sortSpecs->Specs->SortDirection);
					}
				}

				// content
				int fileId = 0;
				for (auto& entry : m_content) {
					std::string filename = entry.Path.filename().string();
					if (filename.size() == 0)
						filename = entry.Path.string(); // drive

					bool isSelected = std::count(m_selections.begin(), m_selections.end(), entry.Path);

					ImGui::TableNextRow();

					// file name
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(m_getIconFont(entry.Path).c_str());
					ImGui::SameLine();
					if (ImGui::Selectable(filename.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
						std::error_code ec;
						bool isDir = std::filesystem::is_directory(entry.Path, ec);

						if (ImGui::IsMouseDoubleClicked(0)) {
							if (isDir) {
								m_setDirectory(entry.Path);
								break;
							} else
								m_finalize(filename);
						} else {
							if ((isDir && m_type == Type::Directory) || !isDir)
								m_select(entry.Path, ImGui::GetIO().KeyCtrl);
						}
					}
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
						m_selectedFileItem = fileId;
					fileId++;

					// date
					ImGui::TableSetColumnIndex(1);
					auto tm = std::localtime(&entry.DateModified);
					if (tm != nullptr)
						ImGui::Text("%d/%d/%d %02d:%02d", tm->tm_mon + 1, tm->tm_mday, 1900 + tm->tm_year, tm->tm_hour, tm->tm_min);
					else
						ImGui::Text("---");

					// size
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%.3f KiB", entry.Size / 1024.0f);
				}

				ImGui::EndTable();
			}
		}
		// "icon" view
		else {
			// content
			int fileId = 0;
			for (auto& entry : m_content) {
				std::string filename = entry.Path.filename().string();
				if (filename.size() == 0)
					filename = entry.Path.string(); // drive

				bool isSelected = std::count(m_selections.begin(), m_selections.end(), entry.Path);

				if (FileIcon(filename.c_str(), isSelected, m_getIconFont(entry.Path).c_str(), ImVec2(32 + 16 * m_zoom, 32 + 16 * m_zoom))) {
					std::error_code ec;
					bool isDir = std::filesystem::is_directory(entry.Path, ec);

					if (ImGui::IsMouseDoubleClicked(0)) {
						if (isDir) {
							m_setDirectory(entry.Path);
							break;
						} else
							m_finalize(filename);
					} else {
						if ((isDir && m_type == Type::Directory) || !isDir)
							m_select(entry.Path, ImGui::GetIO().KeyCtrl);
					}
				}
				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					m_selectedFileItem = fileId;
				fileId++;
			}
		}
	}
	void FileDialog::m_renderPopups() {
		bool openAreYouSureDlg = false, openNewFileDlg = false, openNewDirectoryDlg = false;
		if (ImGui::BeginPopupContextItem("##dir_context")) {
			if (ImGui::Selectable("New file"))
				openNewFileDlg = true;
			if (ImGui::Selectable("New directory"))
				openNewDirectoryDlg = true;
			if (m_selectedFileItem != -1 && ImGui::Selectable("Delete"))
				openAreYouSureDlg = true;
			ImGui::EndPopup();
		}
		if (openAreYouSureDlg)
			ImGui::OpenPopup("Are you sure?##delete");
		if (openNewFileDlg)
			ImGui::OpenPopup("Enter file name##newfile");
		if (openNewDirectoryDlg)
			ImGui::OpenPopup("Enter directory name##newdir");
		if (ImGui::BeginPopupModal("Are you sure?##delete")) {
			if (m_selectedFileItem >= static_cast<int>(m_content.size()) || m_content.size() == 0)
				ImGui::CloseCurrentPopup();
			else {
				const FileData& data = m_content[m_selectedFileItem];
				ImGui::TextWrapped("Are you sure you want to delete %s?", data.Path.filename().string().c_str());
				if (ImGui::Button("Yes")) {
					std::error_code ec;
					std::filesystem::remove_all(data.Path, ec);
					m_setDirectory(m_currentDirectory, false); // refresh
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("No"))
					ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Enter file name##newfile")) {
			ImGui::PushItemWidth(250.0f);
			ImGui::InputText("##newfilename", m_newEntryBuffer, 1024); // TODO: remove hardcoded literals
			ImGui::PopItemWidth();

			if (ImGui::Button("OK")) {
				std::ofstream out((m_currentDirectory / std::string(m_newEntryBuffer)).string());
				out << "";
				out.close();

				m_setDirectory(m_currentDirectory, false); // refresh
				m_newEntryBuffer[0] = 0;

				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				m_newEntryBuffer[0] = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Enter directory name##newdir")) {
			ImGui::PushItemWidth(250.0f);
			ImGui::InputText("##newfilename", m_newEntryBuffer, 1024); // TODO: remove hardcoded literals
			ImGui::PopItemWidth();

			if (ImGui::Button("OK")) {
				std::error_code ec;
				std::filesystem::create_directory(m_currentDirectory / std::string(m_newEntryBuffer), ec);
				m_setDirectory(m_currentDirectory, false); // refresh
				m_newEntryBuffer[0] = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel")) {
				ImGui::CloseCurrentPopup();
				m_newEntryBuffer[0] = 0;
			}
			ImGui::EndPopup();
		}
	}
	// Renders the complete file dialog interior: top navigation bar, split content
	// panel (tree on the left, file list on the right), and the bottom filename/filter bar.
	void FileDialog::m_renderFileDialog() {
		/***** TOP BAR: back / forward / up / path box / favourite star / search *****/
		bool noBackHistory = m_backHistory.empty(), noForwardHistory = m_forwardHistory.empty();

		ImGui::PushStyleColor(ImGuiCol_Button, 0);
		// Back button -- disabled when there is no history to go back to
		ImGui::BeginDisabled(noBackHistory);
		if (ImGui::ArrowButtonEx("##back", ImGuiDir_Left, ImVec2(GUI_ELEMENT_SIZE, GUI_ELEMENT_SIZE))) {
			std::filesystem::path newPath = m_backHistory.top();
			m_backHistory.pop();
			m_forwardHistory.push(m_currentDirectory);

			m_setDirectory(newPath, false);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();

		// Forward button -- disabled when there is no forward history
		ImGui::BeginDisabled(noForwardHistory);
		if (ImGui::ArrowButtonEx("##forward", ImGuiDir_Right, ImVec2(GUI_ELEMENT_SIZE, GUI_ELEMENT_SIZE))) {
			std::filesystem::path newPath = m_forwardHistory.top();
			m_forwardHistory.pop();
			m_backHistory.push(m_currentDirectory);

			m_setDirectory(newPath, false);
		}
		ImGui::EndDisabled();
		ImGui::SameLine();

		// Up button -- navigate to the parent directory
		if (ImGui::ArrowButtonEx("##up", ImGuiDir_Up, ImVec2(GUI_ELEMENT_SIZE, GUI_ELEMENT_SIZE))) {
			if (m_currentDirectory.has_parent_path())
				m_setDirectory(m_currentDirectory.parent_path());
		}

		// Breadcrumb path box -- navigates to any ancestor on click
		std::filesystem::path curDirCopy = m_currentDirectory;
		if (PathBox("##pathbox", curDirCopy, m_pathBuffer, ImVec2(-250, GUI_ELEMENT_SIZE)))
			m_setDirectory(curDirCopy);
		ImGui::SameLine();

		// Favourite star -- toggles the current directory in the Quick Access list
		if (FavoriteButton("##dirfav", std::count(m_favorites.begin(), m_favorites.end(), m_currentDirectory.string()))) {
			if (std::count(m_favorites.begin(), m_favorites.end(), m_currentDirectory.string()))
				RemoveFavorite(m_currentDirectory.string());
			else
				AddFavorite(m_currentDirectory.string());
		}
		ImGui::SameLine();
		ImGui::PopStyleColor();

		// Search box -- filters the content list on every keystroke
		if (ImGui::InputTextEx("##searchTB", "Search", m_searchBuffer, 128, ImVec2(-FLT_MIN, GUI_ELEMENT_SIZE), 0)) // TODO: no hardcoded literals
			m_setDirectory(m_currentDirectory, false);																// refresh

		/***** CONTENT *****/
		float bottomBarHeight = (GImGui->FontSize + ImGui::GetStyle().FramePadding.y + ImGui::GetStyle().ItemSpacing.y * 2.0f) * 2;
		if (ImGui::BeginTable("##table", 2, ImGuiTableFlags_Resizable, ImVec2(0, -bottomBarHeight))) {
			ImGui::TableSetupColumn("##tree", ImGuiTableColumnFlags_WidthFixed, GUI_ELEMENT_SIZE * 12);
			ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			// the tree on the left side
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginChild("##treeContainer", ImVec2(0, -bottomBarHeight));
			for (auto node : m_treeCache)
				m_renderTree(node);
			m_treeSyncRequired = false;
			ImGui::EndChild();

			// content on the right side
			ImGui::TableSetColumnIndex(1);
			ImGui::BeginChild("##contentContainer", ImVec2(0, -bottomBarHeight));
			m_renderContent();
			ImGui::EndChild();
			if (ImGui::IsItemHovered() && ImGui::GetIO().KeyCtrl && ImGui::GetIO().MouseWheel != 0.0f) {
				m_zoom = std::min<float>(25.0f, std::max<float>(1.0f, m_zoom + ImGui::GetIO().MouseWheel));
			}

			// New file, New directory and Delete popups
			m_renderPopups();

			ImGui::EndTable();
		}

		/***** BOTTOM BAR *****/
		ImGui::Text("File name:");
		ImGui::SameLine();
		if (ImGui::InputTextEx("##file_input", "Filename", m_inputTextbox, 1024, ImVec2((m_type != Type::Directory) ? -250.0f : -FLT_MIN, 0),
							   ImGuiInputTextFlags_EnterReturnsTrue)) {
			bool success = m_finalize(std::string(m_inputTextbox));
#ifdef _WIN32
			if (!success)
				MessageBeep(MB_ICONERROR);
#else
			(void)success;
#endif
		}
		if (m_type != Type::Directory) {
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-FLT_MIN);
			int sel = static_cast<int>(m_filterSelection);
			if (ImGui::Combo("##ext_combo", &sel, m_filter.c_str())) {
				m_filterSelection = static_cast<size_t>(sel);
				m_setDirectory(m_currentDirectory, false); // refresh
			}
		}

		// buttons
		float ok_cancel_width = GUI_ELEMENT_SIZE * 7;
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ok_cancel_width);
		if (ImGui::Button(m_type == Type::Save ? "Save" : "Open", ImVec2(ok_cancel_width / 2 - ImGui::GetStyle().ItemSpacing.x, 0.0f))) {
			std::string filename(m_inputTextbox);
			bool success = false;
			if (!filename.empty() || m_type == Type::Directory)
				success = m_finalize(filename);
#ifdef _WIN32
			if (!success)
				MessageBeep(MB_ICONERROR);
#else
			(void)success;
#endif
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 0.0f))) {
			if (m_type == Type::Directory)
				m_isOpen = false;
			else
				m_finalize();
		}

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && ImGuiKey_Escape >= 0 && ImGui::IsKeyPressed(ImGuiKey_Escape))
			m_isOpen = false;
	}
} // namespace ImGui
