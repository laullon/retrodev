//----------------------------------------------------------------------------------------------------
//
//
//
// Taken from: https://github.com/ocornut/imgui/issues/319
//
//----------------------------------------------------------------------------------------------------

#include "imgui.splitter.h"

//
// Draw a splitter between two areas. The splitter can be vertical or horizontal and allows resizing the areas by dragging.
//
void ImGui::DrawSplitter(int split_vertically, float thickness, float* size0, float* size1, float min_size0, float min_size1) {
	ImVec2 backup_pos = ImGui::GetCursorScreenPos();
	//
	// Position the cursor at the start of the splitter area
	//
	if (split_vertically)
		ImGui::SetCursorScreenPos(ImVec2(backup_pos.x, backup_pos.y + *size0));
	else
		ImGui::SetCursorScreenPos(ImVec2(backup_pos.x + *size0, backup_pos.y));
	//
	// Calculate button size (slightly smaller for visual feedback)
	//
	float button_size = thickness - 2.0f;
	float button_offset = (thickness - button_size) * 0.5f;
	//
	// Get available space in perpendicular direction
	//
	ImVec2 avail = ImGui::GetContentRegionAvail();
	//
	// Calculate explicit button dimensions
	//
	ImVec2 button_dims;
	if (split_vertically) {
		//
		// Horizontal bar (divides top/bottom)
		//
		button_dims = ImVec2(avail.x, thickness);
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		cursorPos.y += button_offset;
		ImGui::SetCursorScreenPos(cursorPos);
	} else {
		//
		// Vertical bar (divides left/right)
		//
		button_dims = ImVec2(thickness, avail.y);
		ImVec2 cursorPos = ImGui::GetCursorScreenPos();
		cursorPos.x += button_offset;
		ImGui::SetCursorScreenPos(cursorPos);
	}
	//
	// Draw invisible button for interaction
	//
	if (ImGui::IsItemActive()) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 1.0f, 0, 0.60f));
	} else {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	}
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 1.0f, 0, 0.60f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.8f, 0.10f, 0.60f));
	ImGui::SetNextItemAllowOverlap();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
	ImGui::Button("##Splitter", button_dims);
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(3);
	//
	// Set cursor style when hovering or dragging
	//
	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		if (split_vertically)
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		else
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	//
	// Handle dragging
	//
	if (ImGui::IsItemActive()) {
		float mouse_delta = split_vertically ? ImGui::GetIO().MouseDelta.y : ImGui::GetIO().MouseDelta.x;
		//
		// Apply minimum pane size constraints
		//
		if (mouse_delta < min_size0 - *size0)
			mouse_delta = min_size0 - *size0;
		if (mouse_delta > *size1 - min_size1)
			mouse_delta = *size1 - min_size1;
		//
		// Apply resize
		//
		*size0 += mouse_delta;
		*size1 -= mouse_delta;
	}
	//
	// Restore cursor position
	//
	ImGui::SetCursorScreenPos(backup_pos);
}