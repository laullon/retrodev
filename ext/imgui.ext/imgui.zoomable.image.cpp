//----------------------------------------------------------------------------------------------------
//
//
//
// Taken from:
//
//----------------------------------------------------------------------------------------------------

#include "imgui.zoomable.image.h"
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ImGui {
	//
	// Default parameter values used by the convenience overloads
	//
	static constexpr ImVec2 ZoomableDefaultUV0(0.0f, 0.0f);
	static constexpr ImVec2 ZoomableDefaultUV1(1.0f, 1.0f);
	static constexpr ImVec4 ZoomableDefaultBackgroundColor(0, 0, 0, 0);
	static constexpr ImVec4 ZoomableDefaultTintColor(1, 1, 1, 1);
	//
	// Zoomable image with default parameters
	//
	void Zoomable(ImTextureRef texRef, const ImVec2& displaySize, ZoomableState* state) {
		Zoomable(texRef, displaySize, ZoomableDefaultUV0, ZoomableDefaultUV1, ZoomableDefaultBackgroundColor, ZoomableDefaultTintColor, state);
	}
	//
	// Zoomable image with custom UV coordinates
	//
	void Zoomable(ImTextureRef texRef, const ImVec2& displaySize, const ImVec2& uv0, const ImVec2& uv1, ZoomableState* state) {
		Zoomable(texRef, displaySize, uv0, uv1, ZoomableDefaultBackgroundColor, ZoomableDefaultTintColor, state);
	}
	//
	// Zoomable image with full parameter control
	//
	void Zoomable(ImTextureRef texRef, const ImVec2& imageSize, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& bgColor, const ImVec4& tintColor, ZoomableState* state) {
		//
		// Check image size
		//
		if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
			return;
		//
		// Internal state
		//
		ZoomableState* s = state;
		if (s == nullptr) {
			//
			// We cannot zoom or pan without a state, just show the image
			//
			ImGui::ImageWithBg(texRef, imageSize, uv0, uv1, bgColor, tintColor);
			return;
		}
		//
		// Calculate available space
		//
		ImVec2 availableSize = ImGui::GetContentRegionAvail();
		float infoBarHeight = s->showInfo ? ImGui::GetTextLineHeightWithSpacing() : 0.0f;
		ImVec2 childSize = ImVec2(availableSize.x, availableSize.y - infoBarHeight);
		//
		// Create a child region to limit events to the image area
		// Without the child region, panning the image with the mouse moves the parent window as well
		//
		ImGui::BeginChild("ImageRegion", childSize, false, ImGuiWindowFlags_NoMove);
		//
		// Get texture size
		//
		ImVec2 textureSize = s->textureSize;
		if (textureSize.x <= 0.0f || textureSize.y <= 0.0f) {
			//
			// Use image size as texture size
			//
			textureSize = ImVec2(imageSize.x / std::abs(uv1.x - uv0.x), imageSize.y / std::abs(uv1.y - uv0.y));
		}
		//
		// Compute the aspect-ratio-fitted base size at zoom 1.0
		//
		ImVec2 widgetSize = ImGui::GetContentRegionAvail();
		ImVec2 baseSize = widgetSize;
		if (s->maintainAspectRatio) {
			const float aspectRatio = textureSize.x / textureSize.y;
			if (baseSize.x / baseSize.y > aspectRatio)
				baseSize.x = baseSize.y * aspectRatio;
			else
				baseSize.y = baseSize.x / aspectRatio;
		}
		//
		// Zoom: image scales from baseSize (zoom < 1.0 shrinks, zoom > 1.0 grows)
		//
		const float zoom = s->zoomLevel > 0.0f ? s->zoomLevel : 1.0f;
		ImVec2 zoomedSize = ImVec2(baseSize.x * zoom, baseSize.y * zoom);
		//
		// Display size: clamped to widget per axis independently
		// Aspect ratio is preserved through per-axis UV computation below
		//
		ImVec2 displaySize = ImVec2(std::min(zoomedSize.x, widgetSize.x), std::min(zoomedSize.y, widgetSize.y));
		//
		// Per-axis UV span: what fraction of the texture is visible on each axis
		//
		const float uvSpanX = displaySize.x / zoomedSize.x;
		const float uvSpanY = displaySize.y / zoomedSize.y;
		//
		// Center when smaller than widget, otherwise pin to top-left
		//
		ImVec2 cursorBase = ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY());
		ImVec2 displayPos = ImVec2(displaySize.x < widgetSize.x ? (widgetSize.x - displaySize.x) * 0.5f + cursorBase.x : cursorBase.x,
								   displaySize.y < widgetSize.y ? (widgetSize.y - displaySize.y) * 0.5f + cursorBase.y : cursorBase.y);
		//
		// Set the display position
		//
		ImGui::SetCursorPos(displayPos);
		const ImVec2 screenDisplayPos = ImGui::GetCursorScreenPos();
		//
		// Expose screen-space display rect and UV spans to the caller for overlay use
		//
		s->screenDisplayPos = screenDisplayPos;
		s->screenDisplaySize = displaySize;
		s->uvSpanX = uvSpanX;
		s->uvSpanY = uvSpanY;
		//
		// Draw background texture if provided (tiled, not stretched)
		//
		if (s->backgroundTexture.GetTexID() != ImTextureID_Invalid) {
			ImGui::SetCursorPos(ImVec2(cursorBase.x, cursorBase.y));
			//
			// Calculate UV coordinates to tile the texture
			// Texture is 64x64 pixels (4x4 grid of 16x16 squares)
			// UV > 1.0 causes tiling/wrapping
			//
			ImVec2 bgUV1 = ImVec2(widgetSize.x / 64.0f, widgetSize.y / 64.0f);
			ImGui::Image(s->backgroundTexture, widgetSize, ImVec2(0, 0), bgUV1);
			ImGui::SetCursorPos(displayPos);
		}
		//
		// Apply view setting with per-axis UV spans
		//
		const ImVec2 t1 = ImVec2(s->panOffset.x, s->panOffset.y);
		const ImVec2 uv0New = ImVec2(t1.x + uv0.x * uvSpanX, t1.y + uv0.y * uvSpanY);
		const ImVec2 uv1New = ImVec2(t1.x + uv1.x * uvSpanX, t1.y + uv1.y * uvSpanY);
		//
		// Display the texture only if valid
		// If texture is invalid, just show background (checkerboard)
		//
		if (texRef.GetTexID() != ImTextureID_Invalid) {
			if (s->wrapMode == ZoomableWrapMode::Repeat) {
				// Repeat mode: let UVs exceed [0,1] and rely on the texture sampler to tile
				ImGui::ImageWithBg(texRef, displaySize, uv0New, uv1New, bgColor, tintColor);
			} else {
				// Clamp mode: restrict UV coordinates to [uv0, uv1] and draw only
				// the screen sub-rect that maps to the valid range
				const ImVec2 clampedUv0(std::max(uv0New.x, uv0.x), std::max(uv0New.y, uv0.y));
				const ImVec2 clampedUv1(std::min(uv1New.x, uv1.x), std::min(uv1New.y, uv1.y));
				if (clampedUv1.x > clampedUv0.x && clampedUv1.y > clampedUv0.y) {
					const float uvRangeX = uv1New.x - uv0New.x;
					const float uvRangeY = uv1New.y - uv0New.y;
					const ImVec2 screenMin(screenDisplayPos.x + (clampedUv0.x - uv0New.x) / uvRangeX * displaySize.x,
										   screenDisplayPos.y + (clampedUv0.y - uv0New.y) / uvRangeY * displaySize.y);
					const ImVec2 screenMax(screenDisplayPos.x + (clampedUv1.x - uv0New.x) / uvRangeX * displaySize.x,
										   screenDisplayPos.y + (clampedUv1.y - uv0New.y) / uvRangeY * displaySize.y);
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					if (bgColor.w > 0.0f)
						drawList->AddRectFilled(screenMin, screenMax, ImGui::ColorConvertFloat4ToU32(bgColor));
					drawList->AddImage(texRef, screenMin, screenMax, clampedUv0, clampedUv1, ImGui::ColorConvertFloat4ToU32(tintColor));
				}
				// Advance the layout cursor to keep IsItemHovered() tracking the correct rect
				ImGui::Dummy(displaySize);
			}
		}
		//
		// Draw pixel grid overlay if enabled and zoom level is high enough
		// Use configurable threshold (default 4.0) to allow grid at different zoom levels
		//
		if (s->showPixelGrid && zoom >= s->pixelGridZoomThreshold) {
			//
			// Calculate pixel size in screen space
			//
			float pixelSizeX = displaySize.x / (textureSize.x * uvSpanX);
			float pixelSizeY = displaySize.y / (textureSize.y * uvSpanY);
			//
			// Only draw grid if pixels are large enough to be visible
			// Use logicalSize if set for grid calculation
			//
			const ImVec2 gridSize = (s->logicalSize.x > 0 && s->logicalSize.y > 0) ? s->logicalSize : textureSize;
			if (pixelSizeX >= 4.0f && pixelSizeY >= 4.0f) {
				ImDrawList* drawList = ImGui::GetWindowDrawList();
				//
				// Calculate visible pixel range
				//
				int startPixelX = static_cast<int>(t1.x * gridSize.x);
				int startPixelY = static_cast<int>(t1.y * gridSize.y);
				int endPixelX = static_cast<int>((t1.x + uvSpanX) * gridSize.x) + 1;
				int endPixelY = static_cast<int>((t1.y + uvSpanY) * gridSize.y) + 1;
				//
				// Clamp to grid bounds
				//
				startPixelX = std::max(0, startPixelX);
				startPixelY = std::max(0, startPixelY);
				endPixelX = std::min(static_cast<int>(gridSize.x), endPixelX);
				endPixelY = std::min(static_cast<int>(gridSize.y), endPixelY);
				//
				// Draw vertical grid lines
				//
				for (int px = startPixelX; px <= endPixelX; ++px) {
					float normalizedX = static_cast<float>(px) / gridSize.x;
					float screenX = screenDisplayPos.x + (normalizedX - t1.x) * displaySize.x / uvSpanX;
					ImVec2 lineStart = ImVec2(screenX, screenDisplayPos.y);
					ImVec2 lineEnd = ImVec2(screenX, screenDisplayPos.y + displaySize.y);
					drawList->AddLine(lineStart, lineEnd, IM_COL32(255, 255, 255, 128), 1.0f);
				}
				//
				// Draw horizontal grid lines
				//
				for (int py = startPixelY; py <= endPixelY; ++py) {
					float normalizedY = static_cast<float>(py) / gridSize.y;
					float screenY = screenDisplayPos.y + (normalizedY - t1.y) * displaySize.y / uvSpanY;
					ImVec2 lineStart = ImVec2(screenDisplayPos.x, screenY);
					ImVec2 lineEnd = ImVec2(screenDisplayPos.x + displaySize.x, screenY);
					drawList->AddLine(lineStart, lineEnd, IM_COL32(255, 255, 255, 128), 1.0f);
				}
			}
		}
		//
		// Reset per-frame output flags
		//
		s->clicked = false;
		s->pressed = false;
		//
		// Static state for selection mode
		//
		static ImVec2 selectionStart;
		static bool isSelecting = false;
		if (ImGui::IsItemHovered()) {
			auto& io = ImGui::GetIO();
			//
			// Set cursor based on interaction mode
			//
			if (s->mode == ZoomableMode::Panning) {
				//
				// Hand cursor for panning (open hand when hovering, closed hand when dragging)
				//
				if (io.MouseDown[0] && s->zoomPanEnabled) {
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				} else {
					ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
				}
			} else if (s->mode == ZoomableMode::Picking) {
				//
				// Crosshair cursor for precise picking
				//
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
			} else if (s->mode == ZoomableMode::Selection) {
				//
				// Crosshair cursor for selection mode
				//
				ImGui::SetMouseCursor(ImGuiMouseCursor_None);
			}
			//
			// Update mouse position
			// Use logicalSize if set, otherwise use textureSize
			//
			const ImVec2 coordSize = (s->logicalSize.x > 0 && s->logicalSize.y > 0) ? s->logicalSize : textureSize;
			const ImVec2 screenPoint = ImVec2((io.MousePos.x - screenDisplayPos.x) / displaySize.x, (io.MousePos.y - screenDisplayPos.y) / displaySize.y);
			const ImVec2 imagePoint = ImVec2(t1.x + screenPoint.x * uvSpanX, t1.y + screenPoint.y * uvSpanY);
			s->mousePosition.x = std::clamp(imagePoint.x * coordSize.x, 0.0f, coordSize.x);
			s->mousePosition.y = std::clamp(imagePoint.y * coordSize.y, 0.0f, coordSize.y);
			//
			// Track left button held state regardless of interaction mode
			//
			s->pressed = io.MouseDown[0];
			//
			// Handle interaction based on mode
			//
			if (s->mode == ZoomableMode::Picking) {
				//
				// Picking mode: just detect clicks
				//
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					s->clicked = true;
				}
			} else if (s->mode == ZoomableMode::Selection) {
				//
				// Selection mode: click and drag to select area
				//
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					//
					// Start selection — snap anchor to logical pixel floor when enabled
					//
					selectionStart = s->mousePosition;
					if (s->snapToLogicalPixels) {
						selectionStart.x = std::floor(selectionStart.x);
						selectionStart.y = std::floor(selectionStart.y);
					}
					isSelecting = true;
					s->selectionActive = false;
				}
				if (isSelecting && io.MouseDown[0]) {
					//
					// Snap end corner to logical pixel boundary (draw-time only, not on mousePosition)
					//
					ImVec2 snappedEnd = s->mousePosition;
					if (s->snapToLogicalPixels) {
						snappedEnd.x = std::floor(snappedEnd.x) + 1.0f;
						snappedEnd.y = std::floor(snappedEnd.y) + 1.0f;
					}
					//
					// Update logical selection rect (top-left, width, height)
					//
					float x1 = std::min(selectionStart.x, snappedEnd.x);
					float y1 = std::min(selectionStart.y, snappedEnd.y);
					float x2 = std::max(selectionStart.x, snappedEnd.x);
					float y2 = std::max(selectionStart.y, snappedEnd.y);
					s->selection = ImVec4(x1, y1, x2 - x1, y2 - y1);
					//
					// Compute screen coords from snapped logical corners
					//
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					ImVec2 screenBoxStart = ImVec2(screenDisplayPos.x + (x1 / coordSize.x - t1.x) * displaySize.x / uvSpanX,
												  screenDisplayPos.y + (y1 / coordSize.y - t1.y) * displaySize.y / uvSpanY);
					ImVec2 screenBoxEnd = ImVec2(screenDisplayPos.x + (x2 / coordSize.x - t1.x) * displaySize.x / uvSpanX,
												screenDisplayPos.y + (y2 / coordSize.y - t1.y) * displaySize.y / uvSpanY);
					//
					// Thickness = one logical pixel in screen space
					//
					const float lpxW = displaySize.x / (coordSize.x * uvSpanX);
					const float lpxH = displaySize.y / (coordSize.y * uvSpanY);
					const float snapThickness = std::min(lpxW, lpxH);
					//
					// Solid white rect, alpha blended
					//
					drawList->AddRect(screenBoxStart, screenBoxEnd, IM_COL32(255, 255, 255, 200), 0.0f, 0, snapThickness);
				}
				//
				// Draw crosshair cursor in selection mode
				//
				if (!isSelecting) {
					ImDrawList* drawList = ImGui::GetWindowDrawList();
					const ImVec2 mouseScreenPos = io.MousePos;
					const float crosshairSize = 10.0f;
					//
					// Draw crosshair (white with black outline)
					//
					drawList->AddLine(ImVec2(mouseScreenPos.x - crosshairSize, mouseScreenPos.y), ImVec2(mouseScreenPos.x + crosshairSize, mouseScreenPos.y),
									  IM_COL32(0, 0, 0, 255), 3.0f);
					drawList->AddLine(ImVec2(mouseScreenPos.x - crosshairSize, mouseScreenPos.y), ImVec2(mouseScreenPos.x + crosshairSize, mouseScreenPos.y),
									  IM_COL32(255, 255, 255, 255), 1.0f);
					drawList->AddLine(ImVec2(mouseScreenPos.x, mouseScreenPos.y - crosshairSize), ImVec2(mouseScreenPos.x, mouseScreenPos.y + crosshairSize),
									  IM_COL32(0, 0, 0, 255), 3.0f);
					drawList->AddLine(ImVec2(mouseScreenPos.x, mouseScreenPos.y - crosshairSize), ImVec2(mouseScreenPos.x, mouseScreenPos.y + crosshairSize),
									  IM_COL32(255, 255, 255, 255), 1.0f);
				}
				if (isSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					//
					// End selection - only mark as active if selection has valid size
					//
					isSelecting = false;
					if (s->selection.z > 0.0f && s->selection.w > 0.0f) {
						s->selectionActive = true;
					}
				}
			} else if (s->mode == ZoomableMode::Panning) {
				//
				// Panning mode: click and drag to pan
				//
				if (s->zoomPanEnabled && io.MouseDown[0]) {
					//
					// Pan the image if mouse is moved while pressing the left button
					//
					const ImVec2 screenDelta = ImVec2(io.MouseDelta.x / displaySize.x, io.MouseDelta.y / displaySize.y);
					const ImVec2 imageDelta = ImVec2(screenDelta.x * uvSpanX, screenDelta.y * uvSpanY);
					ImVec2 t2 = ImVec2(t1.x - imageDelta.x, t1.y - imageDelta.y);
					t2.x = std::clamp(t2.x, 0.0f, std::max(0.0f, 1.0f - uvSpanX));
					t2.y = std::clamp(t2.y, 0.0f, std::max(0.0f, 1.0f - uvSpanY));
					//
					// Update translation
					//
					s->panOffset.x = t2.x;
					s->panOffset.y = t2.y;
				}
			}
			//
			// Handle zoom (always available when zoomPanEnabled)
			//
			if (s->zoomPanEnabled) {
				if (io.MouseWheel != 0.0f) {
					//
					// Update image zoom when mouse wheel is scrolled
					// Compute the new zoom
					//
					constexpr float minZoom = 0.1f;
					const float maxZoomLevel = s->maxZoomLevel > 1.0f ? s->maxZoomLevel : std::max(textureSize.x, textureSize.y);
					const float maxScale = 1.0f / minZoom;
					const float minScale = 1.0f / maxZoomLevel;
					const float curScale = 1.0f / zoom;
					const float scaleFactor = io.MouseWheel < 0 ? 1.1f : 0.9f;
					const float newScale = std::min(maxScale, std::max(minScale, scaleFactor * curScale));
					const float newZoom = 1.0f / newScale;
					//
					// Recompute per-axis UV spans for the new zoom level
					//
					ImVec2 newZoomedSize = ImVec2(baseSize.x * newZoom, baseSize.y * newZoom);
					ImVec2 newDisplaySize = ImVec2(std::min(newZoomedSize.x, widgetSize.x), std::min(newZoomedSize.y, widgetSize.y));
					const float newUvSpanX = newDisplaySize.x / newZoomedSize.x;
					const float newUvSpanY = newDisplaySize.y / newZoomedSize.y;
					//
					// Keep the point under the mouse fixed
					//
					ImVec2 t2 = ImVec2(imagePoint.x - screenPoint.x * newUvSpanX, imagePoint.y - screenPoint.y * newUvSpanY);
					t2.x = std::clamp(t2.x, 0.0f, std::max(0.0f, 1.0f - newUvSpanX));
					t2.y = std::clamp(t2.y, 0.0f, std::max(0.0f, 1.0f - newUvSpanY));
					//
					// Update zoom and pan
					//
					s->zoomLevel = newZoom;
					s->panOffset.x = t2.x;
					s->panOffset.y = t2.y;
				}
				//
				// Double-click to reset only in Panning mode
				// (avoid accidental resets in Picking/Selection modes)
				//
				else if (io.MouseDoubleClicked[0] && s->mode == ZoomableMode::Panning) {
					//
					// Reset view on double click
					//
					s->zoomLevel = 1.0f;
					s->panOffset.x = 0.0f;
					s->panOffset.y = 0.0f;
				}
			}
		} else {
			//
			// Make mouse position invalid if the image is not hovered
			//
			s->mousePosition.x = std::numeric_limits<float>::quiet_NaN();
			s->mousePosition.y = std::numeric_limits<float>::quiet_NaN();
		}
		//
		// Draw persistent selection if active (not currently being dragged)
		//
		if (s->selectionActive && !isSelecting && s->selection.z > 0.0f && s->selection.w > 0.0f) {
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 coordSize = (s->logicalSize.x > 0 && s->logicalSize.y > 0) ? s->logicalSize : textureSize;
			//
			// Convert logical selection coordinates to screen coordinates
			//
			ImVec2 screenStart = ImVec2(screenDisplayPos.x + (s->selection.x / coordSize.x - t1.x) * displaySize.x / uvSpanX,
										screenDisplayPos.y + (s->selection.y / coordSize.y - t1.y) * displaySize.y / uvSpanY);
			ImVec2 screenEnd = ImVec2(screenDisplayPos.x + ((s->selection.x + s->selection.z) / coordSize.x - t1.x) * displaySize.x / uvSpanX,
									  screenDisplayPos.y + ((s->selection.y + s->selection.w) / coordSize.y - t1.y) * displaySize.y / uvSpanY);
			//
			// Thickness and dash length = one logical pixel in screen space
			//
			const float lpxW = displaySize.x / (coordSize.x * uvSpanX);
			const float lpxH = displaySize.y / (coordSize.y * uvSpanY);
			const float lineThickness = std::min(lpxW, lpxH);
			const float dashLength = lineThickness;
			//
			// Marching ants: animated dashed border, alpha blended
			//
			static float dashOffset = 0.0f;
			dashOffset += 0.5f;
			if (dashOffset >= dashLength * 2.0f)
				dashOffset = 0.0f;
			//
			// Draw top line (horizontal)
			//
			float x = screenStart.x;
			bool isWhite = (static_cast<int>(dashOffset / dashLength) % 2) == 0;
			while (x < screenEnd.x) {
				float segmentEnd = std::min(x + dashLength, screenEnd.x);
				ImU32 color = isWhite ? IM_COL32(255, 255, 255, 200) : IM_COL32(0, 0, 0, 160);
				drawList->AddLine(ImVec2(x, screenStart.y), ImVec2(segmentEnd, screenStart.y), color, lineThickness);
				x = segmentEnd;
				isWhite = !isWhite;
			}
			//
			// Draw bottom line (horizontal)
			//
			x = screenStart.x;
			isWhite = (static_cast<int>(dashOffset / dashLength) % 2) == 0;
			while (x < screenEnd.x) {
				float segmentEnd = std::min(x + dashLength, screenEnd.x);
				ImU32 color = isWhite ? IM_COL32(255, 255, 255, 200) : IM_COL32(0, 0, 0, 160);
				drawList->AddLine(ImVec2(x, screenEnd.y), ImVec2(segmentEnd, screenEnd.y), color, lineThickness);
				x = segmentEnd;
				isWhite = !isWhite;
			}
			//
			// Draw left line (vertical)
			//
			float y = screenStart.y;
			isWhite = (static_cast<int>(dashOffset / dashLength) % 2) == 0;
			while (y < screenEnd.y) {
				float segmentEnd = std::min(y + dashLength, screenEnd.y);
				ImU32 color = isWhite ? IM_COL32(255, 255, 255, 200) : IM_COL32(0, 0, 0, 160);
				drawList->AddLine(ImVec2(screenStart.x, y), ImVec2(screenStart.x, segmentEnd), color, lineThickness);
				y = segmentEnd;
				isWhite = !isWhite;
			}
			//
			// Draw right line (vertical)
			//
			y = screenStart.y;
			isWhite = (static_cast<int>(dashOffset / dashLength) % 2) == 0;
			while (y < screenEnd.y) {
				float segmentEnd = std::min(y + dashLength, screenEnd.y);
				ImU32 color = isWhite ? IM_COL32(255, 255, 255, 200) : IM_COL32(0, 0, 0, 160);
				drawList->AddLine(ImVec2(screenEnd.x, y), ImVec2(screenEnd.x, segmentEnd), color, lineThickness);
				y = segmentEnd;
				isWhite = !isWhite;
			}
		}
		//
		// End child region
		//
		ImGui::EndChild();
		//
		// Display information bar below the image if enabled
		//
		if (s->showInfo) {
			//
			// Draw semi-transparent background for info text using full widget width
			//
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ImVec2 infoScreenPos = ImGui::GetCursorScreenPos();
			ImVec2 infoBackgroundEnd = ImVec2(infoScreenPos.x + availableSize.x, infoScreenPos.y + infoBarHeight);
			drawList->AddRectFilled(infoScreenPos, infoBackgroundEnd, IM_COL32(0, 0, 0, 180));
			//
			// Left side: name and dimensions (use logical size if available)
			//
			const ImVec2 displayedSize = (s->logicalSize.x > 0 && s->logicalSize.y > 0) ? s->logicalSize : textureSize;
			char leftText[256];
			std::snprintf(leftText, sizeof(leftText), "%s  %.0fx%.0f", s->name.c_str(), displayedSize.x, displayedSize.y);
			ImGui::TextUnformatted(leftText);
			//
			// Right side: coordinates, zoom, and reset button
			//
			char rightText[128];
			if (!std::isnan(s->mousePosition.x) && !std::isnan(s->mousePosition.y))
				std::snprintf(rightText, sizeof(rightText), "Pixel: (%.0f, %.0f)  Zoom: %.0f%%", s->mousePosition.x, s->mousePosition.y, s->zoomLevel * 100.0f);
			else
				std::snprintf(rightText, sizeof(rightText), "Pixel: -  Zoom: %.0f%%", s->zoomLevel * 100.0f);
			float rightTextWidth = ImGui::CalcTextSize(rightText).x;
			float resetButtonWidth = ImGui::CalcTextSize("Reset").x + ImGui::GetStyle().FramePadding.x * 2.0f;
			float totalRightWidth = rightTextWidth + resetButtonWidth + 8.0f;
			ImGui::SameLine(availableSize.x - totalRightWidth - 4.0f);
			ImGui::TextUnformatted(rightText);
			ImGui::SameLine();
			if (ImGui::SmallButton("Reset")) {
				s->zoomLevel = 1.0f;
				s->panOffset.x = 0.0f;
				s->panOffset.y = 0.0f;
			}
		}
	}
} // namespace ImGui
