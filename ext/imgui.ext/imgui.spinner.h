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
	// Animated arc spinner drawn as a partial circle that rotates over time.
	// label        : unique ImGui ID string (not displayed).
	// radius       : outer radius of the spinner in pixels.
	// thickness    : stroke width of the arc in pixels.
	// color        : ABGR packed colour used to stroke the arc.
	// Returns true after the item is added to the window.
	//
	IMGUI_API bool Spinner(const char* label, float radius, int thickness, const ImU32& color);

	//
	// Animated buffering bar: a filled progress bar on the left with three
	// travelling circles on the right that simulate an indeterminate load.
	// label    : unique ImGui ID string (not displayed).
	// value    : fill fraction in [0, 1] for the left bar portion.
	// size_arg : requested widget size in pixels.
	// bg_col   : ABGR colour of the unfilled bar and the travelling circles.
	// fg_col   : ABGR colour of the filled bar portion.
	// Returns true after the item is added to the window.
	//
	IMGUI_API bool BufferingBar(const char* label, float value, const ImVec2& size_arg, const ImU32& bg_col, const ImU32& fg_col);

	//
	// Animated rotating square indicator with an optional alpha fade-in.
	// Draws a filled, rotating quad centred at the current cursor position.
	// label              : unique ImGui ID string (not displayed).
	// started_showing_at : ImGui time value (from ImGui::GetTime()) recorded when
	//                      the indicator first became visible; used to drive the
	//                      fade-in so the widget does not pop in abruptly.
	//
	IMGUI_API void LoadingIndicator(const char* label, double started_showing_at);

	//
	// Animated circular ring of small filled circles whose brightness pulses
	// in sequence to create a "chasing dots" loading animation.
	// label            : unique ImGui ID string (not displayed).
	// indicator_radius : outer radius of the ring in pixels.
	// main_color       : fully-lit colour of each dot at peak brightness.
	// backdrop_color   : dim colour of each dot at minimum brightness.
	// circle_count     : number of dots evenly spaced around the ring.
	// speed            : angular pulse speed (radians per second).
	//
	IMGUI_API void LoadingIndicatorCircle(const char* label, float indicator_radius, const ImVec4& main_color, const ImVec4& backdrop_color, int circle_count, float speed);

}
