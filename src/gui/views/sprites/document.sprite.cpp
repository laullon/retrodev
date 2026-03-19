// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include <app/app.h>
#include <retrodev.lib.h>
#include <widgets/conversion.widget.h>
#include <widgets/palette.widget.h>
#include <widgets/image.viewer.widget.h>
#include <widgets/preview.controls.widget.h>
#include <widgets/sprite.extraction.widget.h>
#include <widgets/sprite.list.widget.h>
#include <widgets/data.export.widget.h>
#include <views/main.view.project.h>
#include <views/main.view.documents.h>
#include "document.sprite.h"
#include <algorithm>

namespace RetrodevGui {
	//
	// The constructor receives the name of the build information and the filename (complete)
	// of the original resource to apply the conversion
	//
	DocumentSprite::DocumentSprite(const std::string& name, const std::string& filepath) : DocumentView(name, filepath) {
		//
		// Create viewer instances (one per tab)
		//
		m_conversionViewer = ImageViewerWidget::Create();
		m_extractionViewer = ImageViewerWidget::Create();
		//
		// We load the original resource
		//
		m_image = RetrodevLib::Image::ImageLoad(filepath);
		if (!m_image) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to load image for sprite: %s", filepath.c_str());
			return;
		}
		ImVec2 texSize = ImVec2((float)m_image->GetWidth(), (float)m_image->GetHeight());
		//
		// Initialize conversion tab zoom states (original image)
		//
		m_conversionZoomStateLeft.textureSize = texSize;
		m_conversionZoomStateLeft.maintainAspectRatio = true;
		m_conversionZoomStateLeft.showInfo = true;
		m_conversionZoomStateLeft.name = name + " (converted)";
		m_conversionZoomStateLeft.showPixelGrid = true;
		m_conversionZoomStateRight.textureSize = texSize;
		m_conversionZoomStateRight.maintainAspectRatio = true;
		//
		// Initialize extraction tab zoom states (will be configured when rendering)
		//
		m_extractionZoomStateLeft.textureSize = texSize;
		m_extractionZoomStateLeft.maintainAspectRatio = true;
		m_extractionZoomStateLeft.showInfo = true;
		m_extractionZoomStateLeft.name = name + " (sprite)";
		m_extractionZoomStateLeft.showPixelGrid = true;
		m_extractionZoomStateRight.textureSize = texSize;
		m_extractionZoomStateRight.maintainAspectRatio = true;
		m_extractionZoomStateRight.showPixelGrid = true;
		m_extractionZoomStateRight.pixelGridZoomThreshold = 1.0f;
		//
		// Query the build configuration data (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::SpriteGetCfg(name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for sprite: %s", name.c_str());
			return;
		}
		//
		// Check if target system is empty (new project or missing configuration)
		// If so, set it to the first available system as default and mark project as modified
		//
		if (params->SParams.TargetSystem.empty()) {
			std::vector<std::string> systems = RetrodevLib::Converters::Get();
			if (!systems.empty()) {
				params->SParams.TargetSystem = systems[0];
				params->SParams.TargetMode = "Mode 0";
				params->SParams.PaletteType = "Hardware";
				params->SParams.ColorSelectionMode = "RGB Clamping";
				params->RParams.TargetResolution = "Normal";
				params->RParams.TargetWidth = 160;
				params->RParams.TargetHeight = 200;
				params->RParams.SourceRect.Width = m_image->GetWidth();
				params->RParams.SourceRect.Height = m_image->GetHeight();
				AppConsole::AddLogF(AppConsole::LogLevel::Info, "Initialized sprite with default settings: %s (system: %s)", name.c_str(), systems[0].c_str());
				RetrodevLib::Project::MarkAsModified();
			} else {
				AppConsole::AddLogF(AppConsole::LogLevel::Error, "No converters available for sprite: %s", name.c_str());
				return;
			}
		}
		//
		// Create the converter for the selected system
		//
		m_converter = RetrodevLib::Converters::GetBitmapConverter(params);
		if (m_converter == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed creating converter for sprite: %s (system: %s)", name.c_str(), params->SParams.TargetSystem.c_str());
			return;
		}
		//
		// Now we can attach the original image to the converter
		//
		m_converter->SetOriginal(m_image);
		//
		// Get sprite extractor from converter
		//
		m_spriteExtractor = m_converter->GetSpriteExtractor();
		//
		// If sprite definitions exist, trigger extraction immediately
		//
		RetrodevLib::SpriteExtractionParams* spriteParams = nullptr;
		if (RetrodevLib::Project::SpriteGetSpriteParams(name, &spriteParams) && spriteParams != nullptr && !spriteParams->Sprites.empty()) {
			//
			// Trigger initial conversion if not done yet
			//
			if (m_converter->GetPreview(params) == nullptr) {
				m_converter->Convert(params);
			}
			//
			// Extract sprites from the CONVERTED image (native resolution), not preview
			//
			auto convertedImage = m_converter->GetConverted(params);
			if (convertedImage && m_spriteExtractor) {
				m_spriteExtractor->Extract(convertedImage, spriteParams);
				AppConsole::AddLogF(AppConsole::LogLevel::Info, "Extracted %d sprites on load from converted image (%dx%d)", m_spriteExtractor->GetSpriteCount(),
									convertedImage->GetWidth(), convertedImage->GetHeight());
			}
		}
		AppConsole::AddLogF(AppConsole::LogLevel::Info, "Sprite document opened successfully: %s", name.c_str());
	}

	DocumentSprite::~DocumentSprite() {}
	//
	// Called when a project build item changes externally (e.g. palette solve validates and
	// updates palette assignments). Reset the converter so it is rebuilt cleanly on the next
	// frame, avoiding dangling raw pointers inside CPCPalette that are invalidated when
	// Validate() reassigns the PaletteLocked/PaletteEnabled vectors in the stored GFXParams.
	//
	void DocumentSprite::OnProjectItemChanged(const std::string& itemName) {
		if (itemName != m_name)
			return;
		m_converter.reset();
		m_previousPreview.reset();
	}
	//
	// Performs the rendering of the sprite document
	//
	void DocumentSprite::Perform() {
		//
		// Show error state if image failed to load
		//
		if (!m_image) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to load image: %s", m_filePath.c_str());
			ImGui::TextWrapped("Please check the console for more details.");
			return;
		}
		//
		// Show error state if converter failed to initialize
		//
		if (!m_converter) {
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Failed to initialize converter");
			ImGui::TextWrapped("Please check the console for more details. You may need to configure the target system in the project settings.");
			return;
		}
		//
		// Tab bar for switching between Conversion and Sprite Extraction
		//
		if (ImGui::BeginTabBar("SpriteDocumentTabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Conversion")) {
				m_activeTab = 0;
				RenderConversionTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Sprite Extraction")) {
				m_activeTab = 1;
				RenderSpriteExtractionTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	//
	// Render the Conversion tab (same as DocumentBitmap/DocumentTiles)
	//
	void DocumentSprite::RenderConversionTab() {
		//
		// Query conversion parameters (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::SpriteGetCfg(m_name, &params) == false || params == nullptr) {
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
					m_conversionZoomStateRight.textureSize = ImVec2((float)m_previousPreview->GetWidth(), (float)m_previousPreview->GetHeight());
					//
					// Set logical size to native resolution for correct pixel grid/coordinates
					//
					m_conversionZoomStateRight.logicalSize = ImVec2((float)m_converter->GetNativeWidth(params), (float)m_converter->GetNativeHeight(params));
					m_conversionZoomStateRight.name = m_name + " (preview)";
					m_conversionZoomStateRight.showInfo = true;
					m_conversionZoomStateRight.showPixelGrid = true;
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
		if (!m_conversionSizesInitialized) {
			m_hSizeRight = fontSize * 25;
			m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_vSizeTop = avail.y * 0.7f;
			m_vSizeBottom = avail.y - (m_vSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			m_imgSizeLeft = m_hSizeLeft * 0.5f;
			m_imgSizeRight = m_hSizeLeft - (m_imgSizeLeft + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_conversionSizesInitialized = true;
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
				m_conversionViewer->SetZoomStates(&m_conversionZoomStateLeft, &m_conversionZoomStateRight);
				m_conversionViewer->SetSplitterSizes(&m_imgSizeLeft, &m_imgSizeRight);
				m_conversionViewer->SetSourceImage(m_image);
				m_conversionViewer->SetConverter(m_converter, params);
				//
				// Preview controls on RIGHT panel (for converted/preview image)
				// Trigger conversion when controls change
				//
				m_conversionViewer->SetPreviewControlsRight(&m_previewAspectCorrection, &m_previewScanlines, true);
				//
				// Enable picking mode only when palette widget is requesting it
				// This ensures picking mode resets back to panning after a pick
				//
				m_conversionViewer->SetPickingEnabled(PaletteWidget::IsPickingFromImage());
				bool pickingOccurred = m_conversionViewer->RenderDualViewer(textureOriginal, texturePreview);
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
			//
			if (m_converter != nullptr) {
				auto result = ConversionWidget::Render(params, m_converter, m_name, RetrodevLib::ProjectBuildType::Sprite);
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
				if (auto previewImage = m_converter->GetPreview(params); previewImage == nullptr) {
					m_converter->Convert(params);
				}
			}
			//
			// Remove left padding
			//
			ImGui::Unindent(8.0f);
		}
		ImGui::EndChild();
	}
	//
	// Render the Sprite Extraction tab
	//
	void DocumentSprite::RenderSpriteExtractionTab() {
		//
		// Query conversion parameters
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::SpriteGetCfg(m_name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for: %s", m_name.c_str());
			return;
		}
		//
		// Query sprite extraction parameters
		//
		RetrodevLib::SpriteExtractionParams* spriteParams = nullptr;
		if (RetrodevLib::Project::SpriteGetSpriteParams(m_name, &spriteParams) == false || spriteParams == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching sprite parameters for: %s", m_name.c_str());
			return;
		}
		//
		// Get the preview image (left panel) - same as conversion tab
		//
		SDL_Texture* texturePreview = nullptr;
		if (m_converter) {
			auto previewImage = m_converter->GetPreview(params);
			//
			// Use double-buffering: only update previous preview when new one is ready
			//
			if (previewImage && previewImage != m_previousPreview) {
				m_previousPreview = previewImage;
			}
			//
			// Use the cached previous preview if available
			//
			if (m_previousPreview) {
				texturePreview = m_previousPreview->GetTexture(Application::GetRenderer());
				//
				// Update zoom state with preview dimensions
				//
				if (texturePreview) {
					m_extractionZoomStateLeft.textureSize = ImVec2((float)m_previousPreview->GetWidth(), (float)m_previousPreview->GetHeight());
					//
					// Set logical size to native resolution for correct pixel grid/coordinates
					//
					m_extractionZoomStateLeft.logicalSize = ImVec2((float)m_converter->GetNativeWidth(params), (float)m_converter->GetNativeHeight(params));
					m_extractionZoomStateLeft.name = m_name + " (preview)";
					m_extractionZoomStateLeft.showInfo = true;
					m_extractionZoomStateLeft.showPixelGrid = true;
				}
			}
		}
		//
		// Right panel: selected sprite preview (cached)
		// Use cached generated preview image stored in the document so its texture stays valid
		SDL_Texture* textureSpritePreview = nullptr;
		if (m_selectedSpriteIndex >= 0 && m_spriteExtractor) {
			m_selectedSpriteImage = m_spriteExtractor->GetSprite(m_selectedSpriteIndex);
			if (m_selectedSpriteImage) {
				// Determine if we need to regenerate the preview
				bool needRegen = false;
				if (!m_selectedSpritePreview)
					needRegen = true;
				if (m_cachedSpritePreviewIndex != m_selectedSpriteIndex)
					needRegen = true;
				if (m_cachedSpritePreviewAspect != m_spritePreviewAspectCorrection)
					needRegen = true;
				if (m_cachedSpritePreviewScanlines != m_spritePreviewScanlines)
					needRegen = true;

				if (needRegen) {
					// Generate preview (only when requested) and store in document member to keep alive
					if (m_converter && m_spritePreviewAspectCorrection) {
						m_selectedSpritePreview = m_converter->GeneratePreview(m_selectedSpriteImage, m_spritePreviewAspectCorrection, m_spritePreviewScanlines);
						if (!m_selectedSpritePreview) {
							AppConsole::AddLogF(AppConsole::LogLevel::Warning, "GeneratePreview failed for sprite #%d, using original", m_selectedSpriteIndex);
							m_selectedSpritePreview = m_selectedSpriteImage;
						}
					} else {
						// No aspect correction requested: use original image as preview
						m_selectedSpritePreview = m_selectedSpriteImage;
					}
					// Update cache state
					m_cachedSpritePreviewIndex = m_selectedSpriteIndex;
					m_cachedSpritePreviewAspect = m_spritePreviewAspectCorrection;
					m_cachedSpritePreviewScanlines = m_spritePreviewScanlines;
				}

				// Use cached preview image to create texture (keeps texture alive across frames)
				if (m_selectedSpritePreview) {
					textureSpritePreview = m_selectedSpritePreview->GetTexture(Application::GetRenderer());
					if (!textureSpritePreview) {
						AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to create texture for sprite #%d (size: %dx%d)", m_selectedSpriteIndex,
											m_selectedSpritePreview->GetWidth(), m_selectedSpritePreview->GetHeight());
					}
				}

				// Configure zoom state for sprite preview
				if (textureSpritePreview && m_selectedSpritePreview) {
					m_extractionZoomStateRight.textureSize = ImVec2((float)m_selectedSpritePreview->GetWidth(), (float)m_selectedSpritePreview->GetHeight());
					// logical size stays as original sprite size (for correct coordinates/grid)
					m_extractionZoomStateRight.logicalSize = ImVec2((float)m_selectedSpriteImage->GetWidth(), (float)m_selectedSpriteImage->GetHeight());
					m_extractionZoomStateRight.name = "Sprite #" + std::to_string(m_selectedSpriteIndex);
					m_extractionZoomStateRight.showInfo = true;
					m_extractionZoomStateRight.showPixelGrid = true;
				}
			}
		}
		//
		// Initialize splitter sizes
		//
		float fontSize = ImGui::GetFontSize();
		ImVec2 avail = ImGui::GetContentRegionAvail();
		if (!m_extractionSizesInitialized) {
			m_hSizeRight = fontSize * 25;
			m_hSizeLeft = avail.x - (m_hSizeRight + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_imgSizeLeft = m_hSizeLeft * 0.5f;
			m_imgSizeRight = m_hSizeLeft - (m_imgSizeLeft + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.x);
			m_extractionVSizeTop = avail.y * 0.6f;
			m_extractionVSizeBottom = avail.y - (m_extractionVSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			m_extractionSizesInitialized = true;
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
		// Left area (sprite viewer + sprite list)
		//
		if (ImGui::BeginChild("SpriteViewerPanel", ImVec2(m_hSizeLeft, 0), false)) {
			ImVec2 leftAvail = ImGui::GetContentRegionAvail();
			m_extractionVSizeBottom = leftAvail.y - (m_extractionVSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			//
			// Vertical splitter: top (image viewer) | bottom (sprite list)
			//
			ImGui::DrawSplitter(true, Application::splitterThickness, &m_extractionVSizeTop, &m_extractionVSizeBottom, vMinTop, vMinBottom);
			//
			// Top: image viewer
			//
			if (ImGui::BeginChild("ExtractionImageViewArea", ImVec2(-1, m_extractionVSizeTop), false)) {
				//
				// Configure image viewer widget using setters
				// Show preview controls in LEFT panel (preview image + controls)
				// Show sprite preview controls in RIGHT panel (sprite + effects)
				// Disable zoom/pan sync (different images: full preview vs selected sprite)
				//
				m_extractionViewer->SetZoomStates(&m_extractionZoomStateLeft, &m_extractionZoomStateRight);
				m_extractionViewer->SetSplitterSizes(&m_imgSizeLeft, &m_imgSizeRight);
				m_extractionViewer->SetSourceImage(m_image);
				m_extractionViewer->SetConverter(m_converter, params);
				//
				// Left panel: preview controls for full image preview (triggers conversion)
				//
				m_extractionViewer->SetPreviewControlsLeft(&m_previewAspectCorrection, &m_previewScanlines, true);
				//
				// Right panel: preview controls for selected sprite (no conversion - uses GeneratePreview)
				//
				m_extractionViewer->SetPreviewControlsRight(&m_spritePreviewAspectCorrection, &m_spritePreviewScanlines, false);
				m_extractionViewer->SetPickingEnabled(false);
				m_extractionViewer->SetSyncZoomPan(false);
				//
				// Enable selection mode if requested
				//
				bool isInSelectionMode = SpriteExtractionWidget::IsSelectionModeActive();
				m_extractionViewer->SetSelectionMode(isInSelectionMode);
				//
				// Only manage selection box when NOT in selection mode
				// During selection mode, let the ZoomableImage widget handle it
				//
				if (!isInSelectionMode) {
					//
					// Show selection box for currently selected sprite
					//
					if (m_selectedSpriteIndex >= 0 && m_selectedSpriteIndex < static_cast<int>(spriteParams->Sprites.size())) {
						const auto& spriteDef = spriteParams->Sprites[m_selectedSpriteIndex];
						m_extractionZoomStateLeft.selection =
							ImVec4(static_cast<float>(spriteDef.X), static_cast<float>(spriteDef.Y), static_cast<float>(spriteDef.Width), static_cast<float>(spriteDef.Height));
						m_extractionZoomStateLeft.selectionActive = true;
					} else {
						m_extractionZoomStateLeft.selectionActive = false;
					}
				}
				m_extractionViewer->RenderDualViewer(texturePreview, textureSpritePreview);
				//
				// Update selection coordinates in widget during dragging
				//
				if (SpriteExtractionWidget::IsSelectionModeActive()) {
					SpriteExtractionWidget::SetSelection(static_cast<int>(m_extractionZoomStateLeft.selection.x), static_cast<int>(m_extractionZoomStateLeft.selection.y),
														 static_cast<int>(m_extractionZoomStateLeft.selection.z), static_cast<int>(m_extractionZoomStateLeft.selection.w));
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
			// Bottom: sprite list panel
			//
			if (ImGui::BeginChild("SpriteListPanel", ImVec2(-1, m_extractionVSizeBottom), true)) {
				auto spriteListResult = SpriteListWidget::Render(m_spriteExtractor, spriteParams, m_selectedSpriteIndex);
				if (spriteListResult.spriteSelected) {
					m_selectedSpriteIndex = spriteListResult.selectedSpriteIndex;
					//
					// When a sprite is selected, show its selection box on the converted image
					//
					if (m_selectedSpriteIndex >= 0 && m_selectedSpriteIndex < static_cast<int>(spriteParams->Sprites.size())) {
						const auto& spriteDef = spriteParams->Sprites[m_selectedSpriteIndex];
						//
						// Set the selection box in the left zoom state (converted image)
						// Selection is stored as (x, y, width, height) in logical coordinates
						//
						m_extractionZoomStateLeft.selection =
							ImVec4(static_cast<float>(spriteDef.X), static_cast<float>(spriteDef.Y), static_cast<float>(spriteDef.Width), static_cast<float>(spriteDef.Height));
						m_extractionZoomStateLeft.selectionActive = true;
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		//
		// Right area: sprite extraction tooling panel
		//
		if (ImGui::BeginChild("SpriteExtractionTooling", ImVec2(m_hSizeRight, 0), true)) {
			//
			// Add left padding to prevent controls from being clipped at edge
			//
			ImGui::Indent(8.0f);
			//
			// 1. Sprite extraction parameters widget
			//
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			if (ImGui::CollapsingHeader("Sprite Extraction")) {
				auto extractionResult = SpriteExtractionWidget::Render(spriteParams, m_spriteExtractor, m_selectedSpriteIndex);
				//
				// Handle extraction parameter changes
				//
				if (extractionResult.parametersChanged) {
					SetModified(true);
					RetrodevLib::Project::MarkAsModified();
				}
				//
				// Trigger extraction if needed
				//
				if (extractionResult.extractionTriggered && m_spriteExtractor && m_converter) {
					auto convertedImage = m_converter->GetConverted(params);
					if (convertedImage) {
						m_spriteExtractor->Extract(convertedImage, spriteParams);
						AppConsole::AddLogF(AppConsole::LogLevel::Info, "Re-extracted %d sprites after parameter change from converted image (%dx%d)",
											m_spriteExtractor->GetSpriteCount(), convertedImage->GetWidth(), convertedImage->GetHeight());
						//
						// Re-fetch the selected sprite from the new array and reset the preview
						// Extract() rebuilds all Image objects so the old pointer is stale
						//
						if (m_selectedSpriteIndex >= 0)
							m_selectedSpriteImage = m_spriteExtractor->GetSprite(m_selectedSpriteIndex);
						m_selectedSpritePreview = nullptr;
						m_cachedSpritePreviewIndex = -1;
					}
				}
				//
				// Check if Done button was clicked (selection mode deactivated but we have a valid selection)
				//
				static bool wasInSelectionMode = false;
				if (wasInSelectionMode && !extractionResult.selectionModeActive) {
					//
					// Read selection coordinates from the widget (not zoom state)
					// This ensures we get the final selection values
					//
					int selX, selY, selWidth, selHeight;
					SpriteExtractionWidget::GetSelection(selX, selY, selWidth, selHeight);
					//
					// Validate selection
					//
					if (selWidth >= 1 && selHeight >= 1) {
						//
						// Add sprite with the selected area
						//
						RetrodevLib::SpriteDefinition newSprite;
						newSprite.X = selX;
						newSprite.Y = selY;
						newSprite.Width = selWidth;
						newSprite.Height = selHeight;
						//
						// Validate minimum size
						//
						if (newSprite.Width < 1)
							newSprite.Width = 1;
						if (newSprite.Height < 1)
							newSprite.Height = 1;
						//
						// Use the name from the extraction widget (user may have edited it)
						//
						newSprite.Name = SpriteExtractionWidget::GetPendingSpriteName();
						//
						// If name is empty, generate default
						//
						if (newSprite.Name.empty()) {
							newSprite.Name = "Sprite" + std::to_string(spriteParams->Sprites.size());
						}
						spriteParams->Sprites.push_back(newSprite);
						//
						// Force conversion if not done yet
						//
						if (!m_previousPreview && m_converter) {
							m_converter->Convert(params);
							auto previewImage = m_converter->GetPreview(params);
							if (previewImage) {
								m_previousPreview = previewImage;
							}
						}
						//
						// Trigger extraction using the CONVERTED image (native resolution), not preview
						//
						if (m_spriteExtractor && m_converter) {
							auto convertedImage = m_converter->GetConverted(params);
							if (convertedImage) {
								m_spriteExtractor->Extract(convertedImage, spriteParams);
								AppConsole::AddLogF(AppConsole::LogLevel::Info, "Extracted %d sprites after adding sprite from converted image (%dx%d)",
													m_spriteExtractor->GetSpriteCount(), convertedImage->GetWidth(), convertedImage->GetHeight());
							}
						}
						//
						// Mark as modified
						//
						SetModified(true);
						RetrodevLib::Project::MarkAsModified();
						//
						// Select the newly added sprite (this will show the selection box on next frame)
						//
						m_selectedSpriteIndex = static_cast<int>(spriteParams->Sprites.size()) - 1;
					}
				}
				wasInSelectionMode = extractionResult.selectionModeActive;
			}
			ImGui::Separator();
			//
			// 2. Export section — script-driven sprite export
			//
			m_exportWidget.Render(RetrodevLib::ProjectBuildType::Sprite, m_name, params, m_converter, nullptr, nullptr, m_spriteExtractor, spriteParams);
			//
			// Remove left padding
			//
			ImGui::Unindent(8.0f);
		}
		ImGui::EndChild();
	}
} // namespace RetrodevGui
