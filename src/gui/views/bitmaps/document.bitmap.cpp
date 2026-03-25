// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Bitmap conversion document -- editor for bitmap conversion build items.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include <app/app.h>
#include <retrodev.lib.h>
#include <widgets/conversion.widget.h>
#include <widgets/palette.widget.h>
#include <widgets/image.viewer.widget.h>
#include <views/main.view.project.h>
#include <views/main.view.documents.h>
#include "document.bitmap.h"

namespace RetrodevGui {

	//
	// The constructor receives the name of the build information and the filename (complete)
	// of the original resource to apply the conversion
	//
	DocumentBitmap::DocumentBitmap(const std::string& name, const std::string& filepath) : DocumentView(name, filepath) {
		//
		// Create viewer instance
		//
		m_imageViewer = ImageViewerWidget::Create();
		//
		// We load the original resource
		//
		m_image = RetrodevLib::Image::ImageLoad(filepath);
		if (m_image) {
			ImVec2 texSize = ImVec2((float)m_image->GetWidth(), (float)m_image->GetHeight());
			m_zoomStateLeft.textureSize = texSize;
			m_zoomStateLeft.maintainAspectRatio = true;
			m_zoomStateLeft.showInfo = true;
			m_zoomStateLeft.name = name + " (original)";
			m_zoomStateLeft.showPixelGrid = true;

			m_zoomStateRight.textureSize = texSize;
			m_zoomStateRight.maintainAspectRatio = true;
		}
		//
		// Query the build configuration data (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::BitmapGetCfg(name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for this bitmap.");
			return;
		}
		//
		// Check if target system is empty (new project or missing configuration)
		// If so, set it to the first available system as default
		//
		if (params->SParams.TargetSystem.empty()) {
			std::vector<std::string> systems = RetrodevLib::Converters::Get();
			if (!systems.empty()) {
				params->SParams.TargetSystem = systems[0];
				AppConsole::AddLogF(AppConsole::LogLevel::Info, "No target system configured, defaulting to: %s", systems[0].c_str());
			}
		}
		//
		// Create the converter for the selected system
		//
		m_converter = RetrodevLib::Converters::GetBitmapConverter(params);
		if (m_converter == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed creating converter for the current selected system");
			return;
		}
		//
		// Now we can attach the original image to the converter
		//
		m_converter->SetOriginal(m_image);
	}

	DocumentBitmap::~DocumentBitmap() {}
	//
	// Called when a project build item changes externally (e.g. palette solve validates and
	// updates palette assignments). Reset the converter so it is rebuilt cleanly on the next
	// frame, avoiding dangling raw pointers inside CPCPalette that are invalidated when
	// Validate() reassigns the PaletteLocked/PaletteEnabled vectors in the stored GFXParams.
	//
	void DocumentBitmap::OnProjectItemChanged(const std::string& itemName) {
		if (itemName != m_name)
			return;
		m_converter.reset();
		m_previousPreview.reset();
	}

	//
	// Performs the rendering of the bitmap conversion
	//
	void DocumentBitmap::Perform() {
		//
		// If the image was not loaded, do it
		//
		if (!m_image) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to load image: %s", m_filePath.c_str());
			return;
		}
		//
		// Query conversion parameters (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::BitmapGetCfg(m_name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for: %s", m_name.c_str());
			return;
		}
		//
		// Rebuild converter if it was reset by OnProjectItemChanged
		//
		if (!m_converter) {
			m_converter = RetrodevLib::Converters::GetBitmapConverter(params);
			if (m_converter) {
				m_converter->SetOriginal(m_image);
				m_converter->Convert(params);
			}
		}
		//
		// Get the texture to be painted (original image)
		//
		SDL_Texture* textureOriginal = m_image->GetTexture(Application::GetRenderer());
		//
		// Check texture was created successfully
		//
		if (!textureOriginal) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to get texture: %s", m_filePath.c_str());
			return;
		}
		//
		// Get the preview texture from the converter if available
		//
		SDL_Texture* texturePreview = nullptr;
		if (m_converter) {
			auto previewImage = m_converter->GetPreview(params);
			//
			// Use double-buffering: only update previous preview when new one is ready
			// This prevents the texture from being destroyed while conversion is in progress
			//
			if (previewImage && previewImage != m_previousPreview) {
				m_previousPreview = previewImage;
			}
			//
			// Use the cached previous preview if available (prevents blinking)
			//
			if (m_previousPreview) {
				texturePreview = m_previousPreview->GetTexture(Application::GetRenderer());
				//
				// Update right zoom state with preview dimensions
				//
				if (texturePreview) {
					m_zoomStateRight.textureSize = ImVec2((float)m_previousPreview->GetWidth(), (float)m_previousPreview->GetHeight());
					//
					// Set logical size to native CPC resolution for correct pixel grid/coordinates
					//
					m_zoomStateRight.logicalSize = ImVec2((float)m_converter->GetNativeWidth(params), (float)m_converter->GetNativeHeight(params));
					m_zoomStateRight.name = m_name + " (preview)";
					m_zoomStateRight.showInfo = true;
					m_zoomStateRight.showPixelGrid = true;
				}
			}
		}
		//
		// If no preview available, fall back to original
		//
		if (!texturePreview)
			texturePreview = textureOriginal;
		//
		// Initialize splitter sizes
		//
		float fontSize = ImGui::GetFontSize();
		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (!m_sizesInitialized) {
			m_hSizeRight = fontSize * 25;
			m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_vSizeTop = avail.y * 0.7f;
			m_vSizeBottom = avail.y - (m_vSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			m_imgSizeLeft = m_hSizeLeft * 0.5f;
			m_imgSizeRight = m_hSizeLeft - (m_imgSizeLeft + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_sizesInitialized = true;
		}
		//
		// Recalculate horizontal sizes when window resizes
		//
		m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
		m_imgSizeRight = m_hSizeLeft - (m_imgSizeLeft + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
		//
		// Minimum sizes for splitter constraints
		//
		float hMinLeft = fontSize * 20;
		float hMinRight = fontSize * 10;
		float vMinTop = fontSize * 8;
		float vMinBottom = fontSize * 5;
		//
		// Horizontal splitter: left area | right area (tooling)
		//
		ImGui::DrawSplitter(false, Application::splitterThickness, &m_hSizeLeft, &m_hSizeRight, hMinLeft, hMinRight);
		//
		// Left area (image viewer + operations)
		//
		if (ImGui::BeginChild("ImageLeftPanel", ImVec2(m_hSizeLeft, 0), false)) {
			ImVec2 leftAvail = ImGui::GetContentRegionAvail();
			m_vSizeBottom = leftAvail.y - (m_vSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			//
			// Vertical splitter: top (image) | bottom (operations)
			//
			ImGui::DrawSplitter(true, Application::splitterThickness, &m_vSizeTop, &m_vSizeBottom, vMinTop, vMinBottom);
			//
			// Top: zoomable image viewer (split into two panels)
			//
			if (ImGui::BeginChild("ImageViewArea", ImVec2(-1, m_vSizeTop), false)) {
				//
				// Configure image viewer widget using setters
				//
				m_imageViewer->SetZoomStates(&m_zoomStateLeft, &m_zoomStateRight);
				m_imageViewer->SetSplitterSizes(&m_imgSizeLeft, &m_imgSizeRight);
				m_imageViewer->SetSourceImage(m_image);
				m_imageViewer->SetConverter(m_converter, params);
				//
				// Preview controls on RIGHT panel (for converted/preview image)
				// Trigger conversion when controls change
				//
				m_imageViewer->SetPreviewControlsRight(&m_previewAspectCorrection, &m_previewScanlines, true);
				//
				// Enable picking mode only when palette widget is requesting it
				// This ensures picking mode resets back to panning after a pick
				//
				m_imageViewer->SetPickingEnabled(PaletteWidget::IsPickingFromImage());
				bool pickingOccurred = m_imageViewer->RenderDualViewer(textureOriginal, texturePreview);
				//
				// Reset picking mode after a successful pick
				//
				if (pickingOccurred) {
					PaletteWidget::CancelColorPicking();
				}
			}
			ImGui::EndChild();
			//
			// Jump the splitter
			//
			ImVec2 SplitterSpace = ImGui::GetCursorScreenPos();
			SplitterSpace.y += Application::splitterThickness;
			ImGui::SetCursorScreenPos(SplitterSpace);
			//
			// Bottom: operations panel
			//
			if (ImGui::BeginChild("PalettePanel", ImVec2(-1, m_vSizeBottom), true)) {
				std::shared_ptr<RetrodevLib::IPaletteConverter> palette = nullptr;
				if (m_converter) {
					palette = m_converter->GetPalette();
					if (PaletteWidget::Render(params, palette)) {
						//
						// Parameters changed, trigger conversion
						//
						m_converter->Convert(params);
						//
						// Mark both document and project as modified
						//
						SetModified(true);
						RetrodevLib::Project::MarkAsModified();
						//
						// Notify open palette documents so they can invalidate their solution
						//
						DocumentsView::NotifyProjectItemChanged(m_name);
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		//
		// Right area: tooling panel (conversion)
		//
		if (ImGui::BeginChild("ToolingPanel", ImVec2(m_hSizeRight, 0), true)) {
			//
			// Add left padding to prevent controls from being clipped at edge
			//
			ImGui::Indent(8.0f);
			//
			// The conversion widget will display the conversion parameters and allow the user to modify them
			// If any parameter has been changed by the user, the converter will be updated with the new parameters
			// and the conversion will be re-applied to the preview image
			//
			// If the parameter changed was the converter itself, then we need to recreate it
			//
			if (m_converter != nullptr) {
				auto result = ConversionWidget::Render(params, m_converter, m_name, RetrodevLib::ProjectBuildType::Bitmap);
				//
				// Handle rename if it occurred
				//
				if (result.itemRenamed) {
					m_name = result.newName;
					//
					// Notify project view to refresh
					//
					ProjectView::NotifyProjectChanged();
					//
					// Mark project as modified (item was renamed)
					//
					RetrodevLib::Project::MarkAsModified();
				}
				//
				// Trigger conversion if parameters changed
				//
				if (result.parametersChanged) {
					m_converter->Convert(params);
					//
					// Mark both document and project as modified
					//
					SetModified(true);
					RetrodevLib::Project::MarkAsModified();
					//
					// Notify open palette documents so they can invalidate their solution
					//
					DocumentsView::NotifyProjectItemChanged(m_name);
				}
				//
				// Force conversion if there is no preview image yet
				// (e.g. first time opening the document or after resetting parameters)
				//
				if (m_converter->GetPreview(params) == nullptr) {
					m_converter->Convert(params);
				}
			}
			ImGui::Separator();
			//
			// Export section -- script-driven bitmap export
			//
			m_exportWidget.Render(RetrodevLib::ProjectBuildType::Bitmap, m_name, params, m_converter);
			//
			// Remove left padding
			//
			ImGui::Unindent(8.0f);
		}
		ImGui::EndChild();
	}
}