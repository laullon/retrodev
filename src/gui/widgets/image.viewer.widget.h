//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <memory>

struct SDL_Texture;

namespace RetrodevGui {
	//
	// Image viewer widget with dual zoomable panels (original + preview)
	// Shared between bitmap, tiles, sprites documents
	// Each instance maintains its own zoom/pan synchronization state
	//
	class ImageViewerWidget {
	public:
		//
		// Factory method to create a new viewer instance
		//
		static std::shared_ptr<ImageViewerWidget> Create();
		//
		// Configuration setters - set these before calling RenderDualViewer
		//
		void SetZoomStates(ImGui::ZoomableState* left, ImGui::ZoomableState* right);
		void SetSplitterSizes(float* sizeLeft, float* sizeRight);
		void SetSourceImage(std::shared_ptr<RetrodevLib::Image> image);
		void SetConverter(std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params);
		void SetPreviewControlsLeft(bool* aspectCorrection, bool* scanlines, bool triggerConversion = false);
		void SetPreviewControlsRight(bool* aspectCorrection, bool* scanlines, bool triggerConversion = false);
		void SetPickingEnabled(bool enabled);
		void SetSyncZoomPan(bool sync);
		void SetSelectionMode(bool enabled);
		//
		// Render dual image viewer (original left, preview right)
		// Returns true if picking occurred
		//
		bool RenderDualViewer(SDL_Texture* textureLeft, SDL_Texture* textureRight);

	private:
		//
		// Private constructor - use Create() factory method
		//
		ImageViewerWidget();
		//
		// Configured state (set via Configure())
		//
		ImGui::ZoomableState* m_zoomStateLeft = nullptr;
		ImGui::ZoomableState* m_zoomStateRight = nullptr;
		float* m_imgSizeLeft = nullptr;
		float* m_imgSizeRight = nullptr;
		bool m_enablePicking = false;
		std::shared_ptr<RetrodevLib::Image> m_sourceImage;
		RetrodevLib::GFXParams* m_params = nullptr;
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_converter;
		//
		// Preview controls for left panel
		//
		bool* m_previewLeftAspectCorrection = nullptr;
		bool* m_previewLeftScanlines = nullptr;
		bool m_previewLeftTriggerConversion = false;
		//
		// Preview controls for right panel
		//
		bool* m_previewRightAspectCorrection = nullptr;
		bool* m_previewRightScanlines = nullptr;
		bool m_previewRightTriggerConversion = false;
		//
		// Instance-specific state
		//
		bool m_syncZoomPan = true;
		bool m_selectionMode = false;
		//
		// Instance-specific zoom/pan synchronization state
		//
		float m_prevZoomLeft = 1.0f;
		float m_prevZoomRight = 1.0f;
		ImVec2 m_prevPanLeft = ImVec2(0, 0);
		ImVec2 m_prevPanRight = ImVec2(0, 0);
		//
		// Cached checkerboard texture for transparency visualization (shared across all instances)
		//
		static SDL_Texture* m_checkerboardTexture;
		//
		// Synchronize zoom and pan between left and right panels
		//
		void SyncZoomPan();
		//
		// Create a checkerboard texture for transparency visualization
		// tileSize: size of each checker square in pixels
		// Returns SDL_Texture* that must be destroyed when done
		//
		static SDL_Texture* CreateCheckerboardTexture(SDL_Renderer* renderer, int tileSize = 8);
		//
		// Get or create the checkerboard background texture (cached)
		//
		static SDL_Texture* GetCheckerboardTexture(SDL_Renderer* renderer);
	};

} // namespace RetrodevGui
