// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "main.view.menu.h"
#include "main.view.documents.h"
#include <app/app.icons.mdi.h>
#include <app/app.console.h>
#include <assets/source/source.h>
#include <assets/source/source.emulator.h>
#include <views/build/document.build.settings.h>
#include <dialogs/dialog.error.h>
#include <dialogs/dialog.about.h>
#include <dialogs/dialog.help.h>
#include <dialogs/dialog.confirm.h>
#include <filesystem>

namespace RetrodevGui {

	//
	// Execute a build for the given build item name.
	// Returns true if the build succeeded, false on error or no params.
	//
	static bool ExecuteBuild(const std::string& buildItemName) {
		RetrodevLib::SourceParams* params = nullptr;
		if (!RetrodevLib::Project::BuildGetParams(buildItemName, &params) || params == nullptr)
			return false;
		//
		// Save any unsaved project/document changes before building
		//
		DocumentsView::SaveAllModified();
		if (RetrodevLib::Project::IsModified()) {
			std::string currentProjectPath = RetrodevLib::Project::GetPath();
			if (RetrodevLib::Project::Save(currentProjectPath))
				RetrodevGui::DocumentsView::ClearAllModifiedFlags();
		}
		AppConsole::Clear(AppConsole::Channel::Build);
		//
		// Set CWD to the project folder so build tools resolve relative paths correctly,
		// then restore it after the build regardless of outcome.
		//
		std::filesystem::path prevCwd = std::filesystem::current_path();
		std::filesystem::path projectDir = std::filesystem::path(RetrodevLib::Project::GetPath()).parent_path();
		std::filesystem::current_path(projectDir);
		//
		// Process all declared dependencies before invoking the assembler
		//
		if (!RetrodevLib::Project::BuildProcessDependencies(buildItemName)) {
			std::filesystem::current_path(prevCwd);
			return false;
		}
		bool ok = RetrodevLib::SourceBuild::Build(params);
		std::filesystem::current_path(prevCwd);
		return ok;
	}
	//
	// Launch the emulator for the given build item, injecting machine-local exe paths.
	//
	static void LaunchEmulator(const std::string& buildItemName) {
		RetrodevLib::SourceParams* dbgParams = nullptr;
		if (!RetrodevLib::Project::BuildGetParams(buildItemName, &dbgParams) || dbgParams == nullptr)
			return;
		RetrodevLib::SourceParams::EmulatorParams& ep = dbgParams->emulatorParams;
		ep.winape.exePath = EmulatorSettings::GetExePath("WinAPE");
		ep.rvm.exePath = EmulatorSettings::GetExePath("RVM");
		ep.acedl.exePath = EmulatorSettings::GetExePath("ACE-DL");
		RetrodevLib::SourceEmulator::Launch(dbgParams);
	}

	void MainViewMenu::RequestQuit() {
		if (RetrodevLib::Project::IsOpen() && RetrodevLib::Project::IsModified()) {
			ConfirmDialog::Show("There are unsaved changes. Quit anyway?", []() {
				m_quitPending = true;
			});
		} else {
			m_quitPending = true;
		}
	}
	//
	// Returns true when the run loop should exit
	//
	bool MainViewMenu::ShouldQuit() {
		return m_quitPending;
	}
	void MainViewMenu::PerformBuildToolbar() {
		const bool projectOpen = RetrodevLib::Project::IsOpen();
		const bool isModified = projectOpen && RetrodevLib::Project::IsModified();
		//
		// Refresh the build item list every frame and validate the stored selection
		//
		std::vector<std::string> buildItems;
		if (projectOpen)
			buildItems = RetrodevLib::Project::GetBuildItemsByType(RetrodevLib::ProjectBuildType::Build);
		std::string selectedItem = projectOpen ? RetrodevLib::Project::GetSelectedBuildItem() : std::string{};
		//
		// If the stored selection no longer exists in the list, fall back to the first item (or clear)
		//
		bool selectionValid = false;
		for (const auto& item : buildItems) {
			if (item == selectedItem) {
				selectionValid = true;
				break;
			}
		}
		if (!selectionValid) {
			selectedItem = buildItems.empty() ? std::string{} : buildItems[0];
			if (projectOpen)
				RetrodevLib::Project::SetSelectedBuildItem(selectedItem);
		}
		const bool canBuild = projectOpen && !selectedItem.empty();
		//
		// Measure right-side block width so we can right-justify it
		//
		float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
		float frameH = ImGui::GetFrameHeight();
		float comboWidth = ImGui::GetFontSize() * 14.0f;
		float buildBtnWidth = ImGui::CalcTextSize(ICON_HAMMER " Build").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float debugBtnWidth = ImGui::CalcTextSize(ICON_BUG_PLAY " Debug").x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float saveBtnWidth = ImGui::CalcTextSize(ICON_CONTENT_SAVE_ALERT).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		//
		// Right block: [save] [combo] [Build] [Debug]  (save only when modified)
		//
		float rightWidth = comboWidth + itemSpacing + buildBtnWidth + itemSpacing + debugBtnWidth;
		if (isModified)
			rightWidth += saveBtnWidth + itemSpacing;
		//
		// Push cursor to align the block against the right edge of the menu bar
		//
		float cursorX = ImGui::GetContentRegionMax().x - rightWidth;
		if (cursorX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(cursorX);
		//
		// Save button — only rendered when there are unsaved changes
		//
		if (isModified) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.10f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.58f, 0.15f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.35f, 0.05f, 1.00f));
			if (ImGui::Button(ICON_CONTENT_SAVE_ALERT)) {
				std::string currentProjectPath = RetrodevLib::Project::GetPath();
				if (RetrodevLib::Project::Save(currentProjectPath))
					DocumentsView::ClearAllModifiedFlags();
			}
			ImGui::PopStyleColor(3);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Unsaved changes — click to save project");
			ImGui::SameLine();
		}
		//
		// Build item combo
		//
		ImGui::BeginDisabled(!projectOpen);
		ImGui::SetNextItemWidth(comboWidth);
		const char* comboPreview = selectedItem.empty() ? "(none)" : selectedItem.c_str();
		if (ImGui::BeginCombo("##BuildItemCombo", comboPreview, ImGuiComboFlags_None)) {
			for (int i = 0; i < (int)buildItems.size(); i++) {
				bool selected = (buildItems[i] == selectedItem);
				if (ImGui::Selectable(buildItems[i].c_str(), selected))
					RetrodevLib::Project::SetSelectedBuildItem(buildItems[i]);
				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(!canBuild);
		if (ImGui::Button(ICON_HAMMER " Build"))
			ExecuteBuild(selectedItem);
		ImGui::SameLine();
		if (ImGui::Button(ICON_BUG_PLAY " Debug")) {
			if (ExecuteBuild(selectedItem))
				LaunchEmulator(selectedItem);
		}
		ImGui::EndDisabled();
		//
		// Suppress unused-variable warning for frameH (used as guard for minimum row height)
		//
		(void)frameH;
	}
	//
	// Returns true if application should quit, false otherwise
	//
	bool MainViewMenu::Perform() {
		if (ImGui::BeginMainMenuBar()) {
			ImGui::Dummy(ImVec2(10.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem(ICON_FILE " New Project", "Ctrl+N")) {
					ImGui::FileDialog::Instance().Save("NewProjectDialog", "New Project", "RetroDev Project (*.retrodev){.retrodev}");
				}
				if (ImGui::MenuItem(ICON_FOLDER_OPEN " Open Project", "Ctrl+O")) {
					ImGui::FileDialog::Instance().Open("OpenProjectDialog", "Open Project", "RetroDev Project (*.retrodev){.retrodev}");
				}
				ImGui::BeginDisabled(!RetrodevLib::Project::IsOpen());
				if (ImGui::MenuItem(ICON_CONTENT_SAVE " Save Project", "Ctrl+S")) {
					//
					// This is just "save" not "save as" so we will save to the current project path, if any
					//
					std::string currentProjectPath = RetrodevLib::Project::GetPath();
					if (RetrodevLib::Project::Save(currentProjectPath)) {
						//
						// Project saved successfully, clear all document modified flags
						//
						DocumentsView::ClearAllModifiedFlags();
					}
				}
				ImGui::EndDisabled();
				ImGui::BeginDisabled(!RetrodevLib::Project::IsOpen());
				if (ImGui::MenuItem(ICON_FOLDER_REMOVE " Close Project")) {
					m_closeProjectPending = true;
				}
				ImGui::EndDisabled();
				if (ImGui::MenuItem(ICON_POWER " Quit")) {
					RequestQuit();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem(ICON_BOOK_OPEN_VARIANT " Documentation"))
					HelpDialog::Show();
				ImGui::Separator();
				if (ImGui::MenuItem(ICON_INFORMATION " About RetroDev"))
					AboutDialog::Show();
				ImGui::EndMenu();
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			//
			// Build toolbar: combo to select a build item, Build and Debug buttons
			// Rendered at the right edge of the menu bar, enabled only when a project is open
			//
			PerformBuildToolbar();
			ImGui::EndMainMenuBar();
		}
		//
		// F5: build the currently selected build item, then launch if successful
		//
		if (ImGui::IsKeyPressed(ImGuiKey_F5, false)) {
			std::string selected = RetrodevLib::Project::GetSelectedBuildItem();
			if (!selected.empty() && ExecuteBuild(selected))
				LaunchEmulator(selected);
		}
		// Handle file dialog results
		if (ImGui::FileDialog::Instance().IsDone("NewProjectDialog")) {
			if (ImGui::FileDialog::Instance().HasResult()) {
				std::string projectPath = ImGui::FileDialog::Instance().GetResult().string();
				if (!RetrodevLib::Project::New(projectPath)) {
					ErrorDialog::Show("Failed to create project:\n" + projectPath);
				}
			}
			ImGui::FileDialog::Instance().Close();
		}
		if (ImGui::FileDialog::Instance().IsDone("OpenProjectDialog")) {
			if (ImGui::FileDialog::Instance().HasResult()) {
				std::string projectPath = ImGui::FileDialog::Instance().GetResult().string();
				if (!RetrodevLib::Project::Open(projectPath)) {
					ErrorDialog::Show("Failed to open project:\n" + projectPath);
				}
			}
			ImGui::FileDialog::Instance().Close();
		}

		ErrorDialog::Render();
		AboutDialog::Render();
		HelpDialog::Render();
		ConfirmDialog::Render();
		//
		// Close Project confirmation modal: opened the frame after the menu item is clicked
		//
		if (m_closeProjectPending) {
			//
			// TODO: check for unsaved changes and ask to save
			//
			ImGui::OpenPopup("Close Project##Modal");
			m_closeProjectPending = false;
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
		if (ImGui::BeginPopupModal("Close Project##Modal", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to close the project?");
			ImGui::Separator();
			if (ImGui::Button(ICON_CHECK " Close", ImVec2(120.0f, 0.0f))) {
				RetrodevLib::Project::Close();
				DocumentsView::CloseAllDocuments();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button(ICON_CLOSE " Cancel", ImVec2(120.0f, 0.0f)))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();

		return m_quitPending;
	}

}