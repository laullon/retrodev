//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#pragma once

#include <imgui.h>

namespace ImGui {

	//
	// Draw a splitter between two areas. The splitter can be vertical or horizontal and allows resizing the areas by dragging.
	//
	IMGUI_API void DrawSplitter(int split_vertically, float thickness, float* size0, float* size1, float min_size0, float min_size1);
} // namespace ImGui
