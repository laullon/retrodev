//----------------------------------------------------------------------------------------------------
//
//
//
//
//
//----------------------------------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <imgui.h>
#include <imgui.spinner.h>
#include <imgui_internal.h>

namespace ImGui {

	//
	// Animated buffering bar.
	// The left portion (0 to circleStart) renders as a two-layer filled rect:
	//   - background colour at full width
	//   - foreground colour scaled by 'value'
	// The right portion (circleStart to circleEnd) renders three circles that
	// travel right-to-left at staggered phase offsets driven by g.Time.
	//
	bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col) {
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) {
			return false;
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		// Subtract horizontal frame padding so the bar aligns with other widgets
		ImVec2 pos = window->DC.CursorPos;
		ImVec2 size = size_arg;
		size.x -= style.FramePadding.x * 2;
		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ItemSize(bb, style.FramePadding.y);
		if (!ItemAdd(bb, id)) {
			return false;
		}
		// Geometry: bar occupies the left 70 %, circle track the right 30 %
		const float circleStart = size.x * 0.7f;
		const float circleEnd = size.x;
		const float circleWidth = circleEnd - circleStart;
		// Draw the background bar then the filled progress bar on top
		window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart, bb.Max.y), bg_col);
		window->DrawList->AddRectFilled(bb.Min, ImVec2(pos.x + circleStart * value, bb.Max.y), fg_col);
		// Compute three travelling circle positions with evenly spaced phase offsets
		const float t = g.Time;
		const float r = size.y / 2;
		const float speed = 1.5f;
		const float b = speed * 0.333f;
		const float c = speed * 0.666f;
		const float o1 = (circleWidth + r) * (t - speed * (int)(t / speed)) / speed;
		const float o2 = (circleWidth + r) * (t + b - speed * (int)((t + b) / speed)) / speed;
		const float o3 = (circleWidth + r) * (t + c - speed * (int)((t + c) / speed)) / speed;
		window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o1, bb.Min.y + r), r, bg_col);
		window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o2, bb.Min.y + r), r, bg_col);
		window->DrawList->AddCircleFilled(ImVec2(pos.x + circleEnd - o3, bb.Min.y + r), r, bg_col);
		return true;
	}

	//
	// Animated arc spinner.
	// Builds a path of num_segments points along an arc whose start angle
	// oscillates via ImSin so the visible arc length varies over time while
	// the whole arc rotates continuously via the g.Time * 8 offset.
	//
	bool Spinner(const char* label, float radius, int thickness, const ImU32& color) {
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) {
			return false;
		}
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		// Reserve a square region tall enough to include vertical frame padding
		ImVec2 pos = window->DC.CursorPos;
		ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);
		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ItemSize(bb, style.FramePadding.y);
		if (!ItemAdd(bb, id)) {
			return false;
		}
		window->DrawList->PathClear();
		// Derive a variable arc start so the spinner length breathes over time
		const int num_segments = 30;
		const int start = (int)ImAbs(ImSin(g.Time * 1.8f) * (num_segments - 5));
		const float a_min = IM_PI * 2.0f * ((float)start) / (float)num_segments;
		const float a_max = IM_PI * 2.0f * ((float)num_segments - 3) / (float)num_segments;
		const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);
		// Build the arc path; the + g.Time * 8 offset drives continuous rotation
		for (int i = 0; i < num_segments; i++) {
			const float a = a_min + ((float)i / (float)num_segments) * (a_max - a_min);
			window->DrawList->PathLineTo(ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
		}
		window->DrawList->PathStroke(color, false, thickness);
		return true;
	}

	//
	// Animated rotating square indicator with alpha fade-in.
	// A filled quad is rotated around the cursor position using ImRotate.
	// The alpha channel fades from 0 to 255 over a fixed 1-second window
	// measured from started_showing_at so the indicator does not pop abruptly.
	// Size is derived from the current font size so it scales with UI density.
	//
	void LoadingIndicator(const char* label, double started_showing_at) {
		ImGuiContext& g = *GImGui;
		// Derive a scale factor from the font size so the indicator is DPI-aware
		const float size = g.FontSize * 0.6f;
		// Place the centre of the quad at the current cursor position
		const ImVec2 cursor = ImGui::GetCursorScreenPos() + ImVec2(size, size);
		// Rotate continuously; one full revolution roughly every 2 seconds
		const float angle = (float)(g.Time * IM_PI);
		const float cosA = ImCos(angle);
		const float sinA = ImSin(angle);
		// Fade in over 1 second from the moment the indicator became visible
		const double elapsed = g.Time - started_showing_at;
		const float fadeIn = (float)ImMin(elapsed, 1.0) ;
		const ImU32 color = IM_COL32(0, 220, 180, (int)(fadeIn * 255.0f));
		// Draw the rotated filled quad
		ImGui::GetWindowDrawList()->AddQuadFilled(
			cursor + ImRotate(ImVec2(-size, -size), cosA, sinA),
			cursor + ImRotate(ImVec2(+size, -size), cosA, sinA),
			cursor + ImRotate(ImVec2(+size, +size), cosA, sinA),
			cursor + ImRotate(ImVec2(-size, +size), cosA, sinA),
			color
		);
		// Advance the cursor so subsequent widgets lay out correctly
		const ImGuiID id = ImGui::GetCurrentWindow()->GetID(label);
		const ImRect bb(cursor - ImVec2(size, size), cursor + ImVec2(size, size));
		ItemSize(bb);
		ItemAdd(bb, id);
	}

	//
	// Animated chasing-dots ring.
	// circle_count evenly-spaced filled circles are drawn around a ring of
	// radius updated_indicator_radius. Each dot's colour is interpolated
	// between backdrop_color and main_color using a sine wave staggered by
	// its angular position, giving the illusion of a pulse travelling around
	// the ring.
	//
	void LoadingIndicatorCircle(const char* label, const float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, const int circle_count,
								const float speed) {
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems) {
			return;
		}
		ImGuiContext& g = *GImGui;
		const ImGuiID id = window->GetID(label);
		const ImVec2 pos = window->DC.CursorPos;
		// Each dot has its own radius; shrink the ring radius to keep dots inside the bounding box
		const float circle_radius = indicator_radius / 15.0f;
		const float updated_indicator_radius = indicator_radius - 4.0f * circle_radius;
		const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
		ItemSize(bb);
		if (!ItemAdd(bb, id)) {
			return;
		}
		const float t = g.Time;
		const float degree_offset = 2.0f * IM_PI / (float)circle_count;
		// Iterate over each dot; compute position and brightness from phase offset
		for (int i = 0; i < circle_count; ++i) {
			const float x = updated_indicator_radius * ImSin(degree_offset * (float)i);
			const float y = updated_indicator_radius * ImCos(degree_offset * (float)i);
			const float growth = ImMax(0.0f, ImSin(t * speed - (float)i * degree_offset));
			// Lerp each channel independently between backdrop and main colour
			ImVec4 color;
			color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
			color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
			color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
			color.w = 1.0f;
			window->DrawList->AddCircleFilled(
				ImVec2(pos.x + indicator_radius + x, pos.y + indicator_radius - y),
				circle_radius + growth * circle_radius,
				GetColorU32(color)
			);
		}
	}

}
