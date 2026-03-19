// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "main.view.h"
#include "main.view.menu.h"
#include "main.view.project.h"
#include "main.view.documents.h"
#include <app/app.console.h>
#include <assets/source/source.emulator.h>
#include <retrodev.gui.h>
#include <imgui_internal.h>

//
// Calcs fails due to child window padding not being taken into account, so we need to adjust the sizes based on the padding and frame sizes.
//

namespace RetrodevGui {

	// Horizontal splitter sizes (project panel | work panel)
	static bool sizesInitialized = false;
	static float size1 = 0;
	static float size2 = 0;
	static float mainSplitterThickness = 5.0f;
	// Vertical splitter sizes inside work panel (documents | bottom panel)
	static float vSizeTop = 0;
	static float vSizeBottom = 0;
	// Collapsible state for the bottom panel
	static bool bottomPanelOpen = false;

	//
	// INI settings handler — persists splitter sizes and bottom panel state
	//
	static void* LayoutHandler_ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char*) {
		return (void*)1;
	}
	static void LayoutHandler_ReadLine(ImGuiContext*, ImGuiSettingsHandler*, void*, const char* line) {
		float f;
		int i;
		// Parse each known key and mark sizes as loaded so defaults are skipped
		if (sscanf(line, "SplitterH=%f", &f) == 1) {
			size1 = f;
			sizesInitialized = true;
		} else if (sscanf(line, "SplitterV=%f", &f) == 1)
			vSizeBottom = f;
		else if (sscanf(line, "BottomOpen=%d", &i) == 1)
			bottomPanelOpen = (i != 0);
	}
	static void LayoutHandler_WriteAll(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf) {
		buf->appendf("[%s][MainView]\n", handler->TypeName);
		buf->appendf("SplitterH=%.3f\n", size1);
		buf->appendf("SplitterV=%.3f\n", vSizeBottom);
		buf->appendf("BottomOpen=%d\n", (int)bottomPanelOpen);
		buf->append("\n");
	}

	void MainView::RegisterSettingsHandler() {
		ImGuiSettingsHandler handler;
		handler.TypeName = "LayoutState";
		handler.TypeHash = ImHashStr("LayoutState");
		handler.ReadOpenFn = LayoutHandler_ReadOpen;
		handler.ReadLineFn = LayoutHandler_ReadLine;
		handler.WriteAllFn = LayoutHandler_WriteAll;
		ImGui::AddSettingsHandler(&handler);
	}

	void MainView::Perform() {
		MainViewMenu::Perform();
		//
		// Drain stdout/stderr from any running emulator processes and detect exit
		//
		RetrodevLib::SourceEmulator::Poll();
		//
		// If a warning or error was logged, uncollapse the bottom panel and switch to its channel
		//
		AppConsole::Channel revealChannel;
		if (AppConsole::TakeRevealRequest(revealChannel)) {
			bottomPanelOpen = true;
			AppConsole::SetActiveChannel(revealChannel);
		}
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		// Get style metrics for proper size calculations
		const ImGuiStyle& style = ImGui::GetStyle();
		float borderSize = style.ChildBorderSize;
		float fontSize = ImGui::GetFontSize();
		// Initialize panel sizes based on font size
		if (!sizesInitialized) {
			size1 = fontSize * 15;
			size2 = viewport->WorkSize.x - (size1 + mainSplitterThickness + ImGui::GetStyle().ItemSpacing.x);
			vSizeBottom = fontSize * 10;
			sizesInitialized = true;
		}
		// Recalculate horizontal sizes when window resizes
		size2 = viewport->WorkSize.x - (size1 + mainSplitterThickness + ImGui::GetStyle().ItemSpacing.x);
		size2 -= style.WindowPadding.x * 2.0f; // Account for horizontal padding in the main window

		// Minimum sizes for splitter constraints
		float minSize1 = fontSize * 10;
		float minSize2 = fontSize * 20;
		float vMinTop = fontSize * 8;
		float vMinBottom = fontSize * 3;
		// Height reserved for the collapsed bottom panel header
		float collapsedHeaderHeight = ImGui::GetFrameHeight();
		// collapsedHeaderHeight += borderSize * 2.0f; // Account for borders in the collapsed state
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
		if (ImGui::Begin("MainFrame", nullptr, flags)) {
			ImVec2 frameAvail = ImGui::GetContentRegionAvail();
			// Horizontal splitter: project panel | work panel
			ImGui::DrawSplitter(false, mainSplitterThickness, &size1, &size2, minSize1, minSize2);
			// ProjectPanel
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
			if (ImGui::BeginChild("ProjectPanel", ImVec2(size1, frameAvail.y), ImGuiChildFlags_Borders)) {
				ProjectView::Perform();
			}
			ImGui::EndChild();
			// WorkPanel (with documents area and collapsible bottom panel)
			ImGui::SameLine();
			// Remove padding for WorkPanel to maximize space
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			if (ImGui::BeginChild("WorkPanel", ImVec2(size2, frameAvail.y), ImGuiChildFlags_None)) {
				ImVec2 workAvail = ImGui::GetContentRegionAvail();
				// Calculate child widths accounting for borders (borders add size on both sides)
				float childWidth = workAvail.x - borderSize * 2.0f;
				if (bottomPanelOpen) {
					// Compute document area height from available space
					// child windows are affected by itemspacing...
					//
					vSizeTop = workAvail.y - (vSizeBottom + mainSplitterThickness + ImGui::GetStyle().ItemSpacing.y);
					if (vSizeTop < vMinTop)
						vSizeTop = vMinTop;
					// Vertical splitter: documents | bottom panel
					ImGui::DrawSplitter(true, mainSplitterThickness, &vSizeTop, &vSizeBottom, vMinTop, vMinBottom);
					// Top Panel
					//
					if (ImGui::BeginChild("DocumentsArea", ImVec2(childWidth, vSizeTop), ImGuiChildFlags_None)) {
						//
						// Top: documents area (accounting for borders)
						//
						DocumentsView::Perform();
					}
					ImGui::EndChild();
					//
					// Jump the splitter
					//
					ImVec2 SplitterSpace = ImGui::GetCursorScreenPos();
					SplitterSpace.y += mainSplitterThickness;
					ImGui::SetCursorScreenPos(SplitterSpace);
					// Bottom panel: collapsible output panel with header inside
					//
					if (ImGui::BeginChild("BottomPanel", ImVec2(childWidth, vSizeBottom), ImGuiChildFlags_Borders,
										  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
						ImGui::SetNextItemOpen(true, ImGuiCond_Always);
						if (ImGui::CollapsingHeader("Output") == false)
							bottomPanelOpen = false;
						//
						// Output panel content goes here
						//
						AppConsole::Render();
					}
					ImGui::EndChild();
				} else {
					// Documents take all space except the collapsed header
					float docsHeight = workAvail.y - (collapsedHeaderHeight + ImGui::GetStyle().ItemSpacing.y);
					// Top: documents area (full size, accounting for borders)
					if (ImGui::BeginChild("DocumentsArea", ImVec2(childWidth, docsHeight), ImGuiChildFlags_None)) {
						DocumentsView::Perform();
					}
					ImGui::EndChild();
					// Collapsed bottom panel: just the header bar
					if (ImGui::BeginChild("BottomPanelCollapsed", ImVec2(childWidth, collapsedHeaderHeight), ImGuiChildFlags_Borders,
										  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
						ImGui::SetNextItemOpen(false, ImGuiCond_Always);
						if (ImGui::CollapsingHeader("Output"))
							bottomPanelOpen = true;
					}
					ImGui::EndChild();
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}
		ImGui::End();
	}
} // namespace RetrodevGui