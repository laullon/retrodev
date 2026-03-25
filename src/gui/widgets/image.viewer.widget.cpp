// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Image viewer widget -- zoomable/pannable image preview panel.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "image.viewer.widget.h"
#include "palette.widget.h"
#include "preview.controls.widget.h"
#include <app/app.h>

namespace RetrodevGui {
	//
	// Static member initialization (only checkerboard texture is shared)
	//
	SDL_Texture* ImageViewerWidget::m_checkerboardTexture = nullptr;
	//
	// Private constructor
	//
	ImageViewerWidget::ImageViewerWidget() {}
	//
	// Factory method to create a new viewer instance
	//
	std::shared_ptr<ImageViewerWidget> ImageViewerWidget::Create() {
		return std::shared_ptr<ImageViewerWidget>(new ImageViewerWidget());
	}
	//
	// Set zoom states for left and right panels
	//
	void ImageViewerWidget::SetZoomStates(ImGui::ZoomableState* left, ImGui::ZoomableState* right) {
		m_zoomStateLeft = left;
		m_zoomStateRight = right;
	}
	//
	// Set splitter sizes for left and right panels
	//
	void ImageViewerWidget::SetSplitterSizes(float* sizeLeft, float* sizeRight) {
		m_imgSizeLeft = sizeLeft;
		m_imgSizeRight = sizeRight;
	}
	//
	// Set source image
	//
	void ImageViewerWidget::SetSourceImage(std::shared_ptr<RetrodevLib::Image> image) {
		m_sourceImage = image;
	}
	//
	// Set converter and parameters
	//
	void ImageViewerWidget::SetConverter(std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params) {
		m_converter = converter;
		m_params = params;
	}
	//
	// Set preview controls for left panel
	//
	void ImageViewerWidget::SetPreviewControlsLeft(bool* aspectCorrection, bool* scanlines, bool triggerConversion) {
		m_previewLeftAspectCorrection = aspectCorrection;
		m_previewLeftScanlines = scanlines;
		m_previewLeftTriggerConversion = triggerConversion;
	}
	//
	// Set preview controls for right panel
	//
	void ImageViewerWidget::SetPreviewControlsRight(bool* aspectCorrection, bool* scanlines, bool triggerConversion) {
		m_previewRightAspectCorrection = aspectCorrection;
		m_previewRightScanlines = scanlines;
		m_previewRightTriggerConversion = triggerConversion;
	}
	//
	// Enable or disable color picking mode
	//
	void ImageViewerWidget::SetPickingEnabled(bool enabled) {
		m_enablePicking = enabled;
		//
		// Update zoom state mode based on picking state
		// Set mode before rendering to ensure correct cursor and behavior
		//
		if (m_zoomStateLeft) {
			m_zoomStateLeft->mode = enabled ? ImGui::ZoomableMode::Picking : ImGui::ZoomableMode::Panning;
		}
	}
	//
	// Enable or disable zoom/pan synchronization between panels
	//
	void ImageViewerWidget::SetSyncZoomPan(bool sync) {
		m_syncZoomPan = sync;
	}
	//
	// Enable or disable selection mode for rectangular sprite selection
	//
	void ImageViewerWidget::SetSelectionMode(bool enabled) {
		m_selectionMode = enabled;
		//
		// Update zoom state mode based on selection state
		//
		if (m_zoomStateLeft) {
			m_zoomStateLeft->mode = enabled ? ImGui::ZoomableMode::Selection : ImGui::ZoomableMode::Panning;
		}
	}
	//
	// Create a checkerboard texture for transparency visualization
	//
	SDL_Texture* ImageViewerWidget::CreateCheckerboardTexture(SDL_Renderer* renderer, int tileSize) {
		//
		// Create a small repeating pattern (4x4 tiles)
		// Small texture for better performance and proper tiling
		//
		const int numTiles = 4;
		const int textureSize = tileSize * numTiles;
		const uint8_t lightGray = 204;
		const uint8_t darkGray = 153;
		//
		// Create texture
		//
		SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, textureSize, textureSize);
		if (!texture)
			return nullptr;
		//
		// Set blend mode for proper transparency handling
		//
		SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
		//
		// Render to texture
		//
		SDL_SetRenderTarget(renderer, texture);
		//
		// Draw checkerboard pattern
		//
		for (int tileY = 0; tileY < numTiles; tileY++) {
			for (int tileX = 0; tileX < numTiles; tileX++) {
				//
				// Alternate colors in checkerboard pattern
				//
				uint8_t color = ((tileX + tileY) % 2 == 0) ? lightGray : darkGray;
				SDL_SetRenderDrawColor(renderer, color, color, color, 255);
				SDL_FRect rect = {(float)(tileX * tileSize), (float)(tileY * tileSize), (float)tileSize, (float)tileSize};
				SDL_RenderFillRect(renderer, &rect);
			}
		}
		//
		// Restore render target
		//
		SDL_SetRenderTarget(renderer, nullptr);
		return texture;
	}
	//
	// Get or create the checkerboard background texture (cached)
	//
	SDL_Texture* ImageViewerWidget::GetCheckerboardTexture(SDL_Renderer* renderer) {
		if (!m_checkerboardTexture) {
			m_checkerboardTexture = CreateCheckerboardTexture(renderer, 16);
		}
		return m_checkerboardTexture;
	}
	//
	// Render dual image viewer with original (left) and preview (right) panels
	//
	bool ImageViewerWidget::RenderDualViewer(SDL_Texture* textureLeft, SDL_Texture* textureRight) {
		bool pickingOccurred = false;
		float imgMinLeft = ImGui::GetFontSize() * 10;
		float imgMinRight = ImGui::GetFontSize() * 10;
		//
		// Get checkerboard background texture for transparency visualization
		//
		SDL_Texture* checkerboard = GetCheckerboardTexture(Application::GetRenderer());
		//
		// Set background texture in zoom states if available
		//
		if (checkerboard) {
			m_zoomStateLeft->backgroundTexture = (ImTextureRef)checkerboard;
			m_zoomStateRight->backgroundTexture = (ImTextureRef)checkerboard;
		}
		//
		// Horizontal splitter for left and right image panels
		//
		ImGui::DrawSplitter(false, Application::splitterThickness, m_imgSizeLeft, m_imgSizeRight, imgMinLeft, imgMinRight);
		//
		// Left image panel
		//
		if (ImGui::BeginChild("ImageViewPanelLeft", ImVec2(*m_imgSizeLeft, -1), true)) {
			//
			// Show preview controls in left panel if configured
			//
			if (m_previewLeftAspectCorrection && m_previewLeftScanlines) {
				if (PreviewControlsWidget::RenderSimple(m_previewLeftAspectCorrection, m_previewLeftScanlines)) {
					//
					// Preview controls changed - trigger conversion only if configured
					//
					if (m_previewLeftTriggerConversion && m_converter && m_params) {
						m_converter->SetPreviewParams(*m_previewLeftAspectCorrection, 1, *m_previewLeftScanlines);
						m_converter->Convert(m_params);
					}
				}
			}
			//
			// Show compact picking mode indicator
			//
			if (m_enablePicking && PaletteWidget::IsPickingFromImage()) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
				ImGui::TextUnformatted("Click on image to pick color");
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::SmallButton("Cancel")) {
					PaletteWidget::CancelColorPicking();
				}
			}
			ImVec2 displaySize = ImGui::GetContentRegionAvail();
			ImGui::Zoomable(textureLeft, displaySize, m_zoomStateLeft);
			//
			// Check if picking transparent color from image using Zoomable's clicked flag
			//
			if (m_enablePicking && PaletteWidget::IsPickingFromImage() && m_zoomStateLeft->clicked) {
				//
				// Get pixel coordinates from Zoomable's mousePosition (already in texture space)
				//
				int pixelX = (int)m_zoomStateLeft->mousePosition.x;
				int pixelY = (int)m_zoomStateLeft->mousePosition.y;
				//
				// Clamp to image bounds
				//
				pixelX = std::max(0, std::min(pixelX, m_sourceImage->GetWidth() - 1));
				pixelY = std::max(0, std::min(pixelY, m_sourceImage->GetHeight() - 1));
				//
				// Get pixel color from source image
				//
				RetrodevLib::RgbColor pickedColor = m_sourceImage->GetPixelColor(pixelX, pixelY);
				//
				// Set the picked color
				//
				PaletteWidget::SetPickedColor(pickedColor.r, pickedColor.g, pickedColor.b, m_params);
				//
				// Trigger conversion since params changed
				//
				if (m_converter) {
					m_converter->Convert(m_params);
				}
				pickingOccurred = true;
			}
		}
		ImGui::EndChild();
		ImGui::SameLine();
		//
		// Right image panel
		//
		if (ImGui::BeginChild("ImageViewPanelRight", ImVec2(*m_imgSizeRight, -1), true)) {
			//
			// Show preview controls in right panel if configured
			//
			if (m_previewRightAspectCorrection && m_previewRightScanlines) {
				if (PreviewControlsWidget::RenderSimple(m_previewRightAspectCorrection, m_previewRightScanlines)) {
					//
					// Preview controls changed - trigger conversion only if configured
					//
					if (m_previewRightTriggerConversion && m_converter && m_params) {
						m_converter->SetPreviewParams(*m_previewRightAspectCorrection, 1, *m_previewRightScanlines);
						m_converter->Convert(m_params);
					}
				}
			}
			//
			// Zoomable preview
			//
			ImVec2 displaySize = ImGui::GetContentRegionAvail();
			ImGui::Zoomable(textureRight, displaySize, m_zoomStateRight);
		}
		ImGui::EndChild();
		//
		// Synchronize zoom and pan between the two panels (only if enabled)
		//
		if (m_syncZoomPan) {
			SyncZoomPan();
		}
		return pickingOccurred;
	}
	//
	// Synchronize zoom and pan between left and right image panels
	// Shows the same area of the image in both views
	//
	void ImageViewerWidget::SyncZoomPan() {
		//
		// Check if left panel zoom/pan changed
		//
		bool leftChanged = (m_zoomStateLeft->zoomLevel != m_prevZoomLeft || m_zoomStateLeft->panOffset.x != m_prevPanLeft.x || m_zoomStateLeft->panOffset.y != m_prevPanLeft.y);
		//
		// Check if right panel zoom/pan changed
		//
		bool rightChanged =
			(m_zoomStateRight->zoomLevel != m_prevZoomRight || m_zoomStateRight->panOffset.x != m_prevPanRight.x || m_zoomStateRight->panOffset.y != m_prevPanRight.y);
		//
		// If both changed (unlikely), prefer left
		//
		if (leftChanged && rightChanged)
			rightChanged = false;
		//
		// Sync from left to right
		//
		if (leftChanged) {
			//
			// Copy zoom level and pan offset directly
			// Both represent the same relative area of the image regardless of resolution
			//
			m_zoomStateRight->zoomLevel = m_zoomStateLeft->zoomLevel;
			m_zoomStateRight->panOffset = m_zoomStateLeft->panOffset;
		}
		//
		// Sync from right to left
		//
		if (rightChanged) {
			//
			// Copy zoom level and pan offset directly
			//
			m_zoomStateLeft->zoomLevel = m_zoomStateRight->zoomLevel;
			m_zoomStateLeft->panOffset = m_zoomStateRight->panOffset;
		}
		//
		// Update previous states for next frame
		//
		m_prevZoomLeft = m_zoomStateLeft->zoomLevel;
		m_prevZoomRight = m_zoomStateRight->zoomLevel;
		m_prevPanLeft = m_zoomStateLeft->panOffset;
		m_prevPanRight = m_zoomStateRight->panOffset;
	}
}
