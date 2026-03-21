//----------------------------------------------------------------------------------------------------
//
// ImGui Zoomable Image Widget
//
// Based on imgui_zoomable_image by Daniel Moreno — https://github.com/danielm5/imgui_zoomable_image
// Drop-in extension of ImGui::Image() adding zoom, pan, pixel grid and interaction modes.
//
// (c) TLOTB 2026
//
//----------------------------------------------------------------------------------------------------

// USAGE
// ---------------------------------------------------------------------------------------------------
//
// 1. Minimal — no zoom/pan persistence, state discarded each frame:
//
//        ImGui::Zoomable(texRef, ImVec2(width, height));
//
// 2. Persistent zoom and pan — allocate ZoomableState once and keep it alive:
//
//        static ImGui::ZoomableState state;
//        ImGui::Zoomable(texRef, ImVec2(width, height), &state);
//
// 3. Full control — UV range, background color, tint color and state:
//
//        static ImGui::ZoomableState state;
//        ImGui::Zoomable(texRef, ImVec2(width, height), uv0, uv1, bgColor, tintColor, &state);
//
// Key state inputs (set before calling Zoomable):
//
//        state.textureSize         = ImVec2(w, h);               // required for aspect ratio and pixel grid
//        state.maintainAspectRatio = true;                        // preserve image aspect ratio on resize
//        state.showInfo            = true;                        // info bar: name, resolution, zoom, pixel coords
//        state.showPixelGrid       = true;                        // pixel grid overlay when sufficiently zoomed in
//        state.mode     = ImGui::ZoomableMode::Panning;           // default: drag to pan, scroll to zoom, double-click to reset
//        state.mode     = ImGui::ZoomableMode::Picking;           // click  → state.clicked + state.mousePosition
//        state.mode     = ImGui::ZoomableMode::Selection;         // drag   → state.selectionActive + state.selection
//        state.wrapMode = ImGui::ZoomableWrapMode::Clamp;         // default: UV coordinates clamped to [0,1]
//        state.wrapMode = ImGui::ZoomableWrapMode::Repeat;        // UV coordinates wrap; texture tiles
//
// Key state outputs (read after calling Zoomable):
//
//        state.zoomLevel       — current zoom factor (1.0 = 100%)
//        state.mousePosition   — pixel coordinate under cursor, or NaN when outside the image
//        state.clicked         — true on the frame the user clicked (Picking mode)
//        state.selection       — ImVec4(x, y, w, h) in logical pixel coordinates (Selection mode)
//        state.selectionActive — true once a completed drag-selection exists
//
#pragma once

#include "imgui.h"
#include <string>

namespace ImGui {
	//
	// Interaction mode for the zoomable image widget
	// Defines how mouse clicks and drags are interpreted
	//
	enum class ZoomableMode {
		Panning,  // Click and drag to pan the image (default)
		Picking,  // Click to pick/select a pixel location
		Selection // Click and drag to select a rectangular area (XOR box)
	};
	//
	// Texture wrapping mode for the main texture
	// Controls how UV coordinates that fall outside [0,1] are handled during rendering
	//
	enum class ZoomableWrapMode {
		Clamp, // UV coordinates are clamped to [0,1]; texture does not repeat (default)
		Repeat // UV coordinates wrap; the texture tiles when the visible area exceeds texture bounds
	};
	//
	// Structure to hold the state of the zoomable image widget
	// You can create an instance of this structure and pass it to the
	// Zoomable() function to maintain the zoom and pan state across frames
	//
	// Members:
	// - Inputs (not modified by the widget):
	//   - zoomPanEnabled: Enable or disable zooming and panning functionality
	//   - maintainAspectRatio: Maintain the aspect ratio of the image when resizing. Requires textureSize to be set
	//   - showInfo: Display information overlay at the bottom of the image (name, resolution, zoom, coordinates)
	//   - showPixelGrid: Display a pixel grid overlay when zoomed in (helps visualize individual pixels)
	//   - maxZoomLevel: Maximum allowed zoom level (0.0 = automatically set)
	//   - pixelGridZoomThreshold: Minimum zoom level to show pixel grid (default 4.0)
	//   - mode: Interaction mode (Panning, Picking, or Selection)
	//   - textureSize: Size of the texture/image being displayed. This is the original size of the image in pixels
	//                  If not set, the widget will attempt to infer the size from the displayed image size and UV coordinates
	//   - name: Name to display in the information overlay (shown on the left side)
	//   - backgroundTexture: Optional background texture to display behind the main texture (useful for showing transparency)
	//   - wrapMode: Controls how UV coordinates outside [0,1] are handled (Clamp = default, Repeat = tile)
	//   - logicalSize: Optional logical pixel size; if set, mouse coordinates and pixel grid use this instead of textureSize
	// - Outputs (set by the widget):
	//   - zoomLevel: Current zoom level (1.0 = 100%)
	//   - panOffset: Current pan offset in normalized coordinates (-1.0 to 1.0)
	//   - mousePosition: Current mouse position within the image area, or NaN if the mouse is outside the image area
	//   - clicked: Set to true when the image was clicked this frame (picking mode)
	//   - selection: Selected area in logical coordinates (selection mode)
	//   - selectionActive: True if a selection is currently being made
	//
	struct ZoomableState {
		//
		// User Inputs
		//
		bool zoomPanEnabled = true;
		bool maintainAspectRatio = false;
		bool showInfo = false;
		bool showPixelGrid = false;
		float maxZoomLevel = 0.0f;
		float pixelGridZoomThreshold = 4.0f;
		ZoomableMode mode = ZoomableMode::Panning;
		ImVec2 textureSize = ImVec2(0.0f, 0.0f);
		std::string name;
		ImTextureRef backgroundTexture = nullptr;
		ZoomableWrapMode wrapMode = ZoomableWrapMode::Clamp;
		//
		// Logical size for pixel grid and coordinates
		// If set, mouse position and pixel grid are calculated using this size instead of textureSize
		// Useful when the texture is scaled (e.g., aspect ratio correction) but you want to show original coordinates
		//
		ImVec2 logicalSize = ImVec2(0.0f, 0.0f);
		//
		// When true, mouse position and selection corners are snapped to logical pixel boundaries
		// Ensures selections align exactly to the pixel grid of the target system
		//
		bool snapToLogicalPixels = false;
		//
		// Outputs
		//
		float zoomLevel = 1.0f;
		ImVec2 panOffset = ImVec2(0.0f, 0.0f);
		ImVec2 mousePosition = ImVec2(0.0f, 0.0f);
		bool clicked = false;
		bool pressed = false;
		//
		// Screen-space position and size of the displayed image rect (set each frame)
		// Used by callers to convert image pixel coords back to screen coords for overlays
		//
		ImVec2 screenDisplayPos = ImVec2(0.0f, 0.0f);
		ImVec2 screenDisplaySize = ImVec2(0.0f, 0.0f);
		float uvSpanX = 1.0f;
		float uvSpanY = 1.0f;
		//
		// Selection mode outputs (in logical coordinates)
		//
		ImVec4 selection = ImVec4(0.0f, 0.0f, 0.0f, 0.0f); // x, y, width, height
		bool selectionActive = false;
	};
	//
	// Zoomable image display functions
	// There are three overloads of the Zoomable() function provided for convenience
	// The first two overloads use default values for the UV coordinates, background color, and tint color
	// The third overload allows you to specify all parameters explicitly
	// All overloads accept an optional ZoomableState pointer to maintain the zoom and pan state across frames
	//
	// Use ImGuiImage::Zoomable() as a drop-in replacement for ImGui::Image() to add zooming and panning functionality
	//
	// To learn more about ImTextureRef, UV coordinates, and other parameters, refer to the ImGui documentation:
	// https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
	//
	// Parameters:
	// - texRef: The ImGui texture reference of the image to display
	// - displaySize: The size (width and height) to display the image within the ImGui window
	// - uv0: The UV coordinates of the top-left corner of the image
	// - uv1: The UV coordinates of the bottom-right corner of the image
	// - bgColor: Background color behind the image
	// - tintColor: Tint color to apply to the image
	// - state: Optional pointer to a ZoomableState structure to maintain zoom and pan state across frames. If nullptr, no state is maintained
	//
	IMGUI_API void Zoomable(ImTextureRef texRef, const ImVec2& displaySize, ZoomableState* state = nullptr);
	IMGUI_API void Zoomable(ImTextureRef texRef, const ImVec2& displaySize, const ImVec2& uv0, const ImVec2& uv1, ZoomableState* state = nullptr);
	IMGUI_API void Zoomable(ImTextureRef texRef, const ImVec2& displaySize, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bgColor, const ImVec4& tintColor,
							ZoomableState* state = nullptr);
} // namespace ImGui
