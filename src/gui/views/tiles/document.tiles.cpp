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
#include <widgets/tile.extraction.widget.h>
#include <widgets/tile.list.widget.h>
#include <views/main.view.project.h>
#include <views/main.view.documents.h>
#include <convert/amstrad.cpc/cpc.tileset.h>
#include "document.tiles.h"
#include <algorithm>

namespace RetrodevGui {
	//
	// The constructor receives the name of the build information and the filename (complete)
	// of the original resource to apply the conversion
	//
	DocumentTiles::DocumentTiles(const std::string& name, const std::string& filepath) : DocumentView(name, filepath) {
		//
		// Create viewer instances (one per tab)
		//
		m_conversionViewer = ImageViewerWidget::Create();
		m_extractionViewer = ImageViewerWidget::Create();
		//
		// We load the original resource
		//
		m_image = RetrodevLib::Image::ImageLoad(filepath);
		if (m_image) {
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
			m_extractionZoomStateLeft.name = name + " (tile)";
			m_extractionZoomStateLeft.showPixelGrid = true;
			m_extractionZoomStateRight.textureSize = texSize;
			m_extractionZoomStateRight.maintainAspectRatio = true;
			m_extractionZoomStateRight.showPixelGrid = true;
			m_extractionZoomStateRight.pixelGridZoomThreshold = 1.0f;
		}
		//
		// Query the build configuration data (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::TilesetGetCfg(name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for this tileset.");
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
		//
		// Get tile extractor from converter (lazy initialization)
		//
		m_tileExtractor = m_converter->GetTileExtractor();
	}

	DocumentTiles::~DocumentTiles() {}
	//
	// Called when a project build item changes externally (e.g. palette solve validates and
	// updates palette assignments). Reset the converter so it is rebuilt cleanly on the next
	// frame, avoiding dangling raw pointers inside CPCPalette that are invalidated when
	// Validate() reassigns the PaletteLocked/PaletteEnabled vectors in the stored GFXParams.
	//
	void DocumentTiles::OnProjectItemChanged(const std::string& itemName) {
		if (itemName != m_name)
			return;
		m_converter.reset();
		m_previousPreview.reset();
	}
	//
	// Performs the rendering of the tiles document
	//
	void DocumentTiles::Perform() {
		//
		// If the image was not loaded, do it
		//
		if (!m_image) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to load image: %s", m_filePath.c_str());
			return;
		}
		//
		// Tab bar for switching between Conversion and Tile Extraction
		//
		if (ImGui::BeginTabBar("TilesDocumentTabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Conversion")) {
				m_activeTab = 0;
				RenderConversionTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Tile Extraction")) {
				m_activeTab = 1;
				RenderTileExtractionTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	//
	// Render the Conversion tab (same as DocumentBitmap)
	//
	void DocumentTiles::RenderConversionTab() {
		//
		// Query conversion parameters (do not store pointer as it can become invalid)
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::TilesetGetCfg(m_name, &params) == false || params == nullptr) {
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
				auto result = ConversionWidget::Render(params, m_converter, m_name, RetrodevLib::ProjectBuildType::Tilemap);
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
	// Render the Tile Extraction tab
	//
	void DocumentTiles::RenderTileExtractionTab() {
		//
		// Query conversion parameters
		//
		RetrodevLib::GFXParams* params = nullptr;
		if (RetrodevLib::Project::TilesetGetCfg(m_name, &params) == false || params == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching conversion parameters for: %s", m_name.c_str());
			return;
		}
		//
		// Query tile extraction parameters
		//
		RetrodevLib::TileExtractionParams* tileParams = nullptr;
		if (RetrodevLib::Project::TilesetGetTileParams(m_name, &tileParams) == false || tileParams == nullptr) {
			AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed fetching tile parameters for: %s", m_name.c_str());
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
		// Right panel: selected tile preview (cached)
		SDL_Texture* textureTilePreview = nullptr;
		if (m_selectedTileIndex >= 0 && m_tileExtractor) {
			// Get the selected tile image
			m_selectedTileImage = m_tileExtractor->GetTile(m_selectedTileIndex);
			if (m_selectedTileImage) {
				// Determine if we need to regenerate the preview
				bool needRegen = false;
				if (!m_selectedTilePreview)
					needRegen = true;
				if (m_cachedTilePreviewIndex != m_selectedTileIndex)
					needRegen = true;
				if (m_cachedTilePreviewAspect != m_tilePreviewAspectCorrection)
					needRegen = true;
				if (m_cachedTilePreviewScanlines != m_tilePreviewScanlines)
					needRegen = true;

				if (needRegen) {
					if (m_converter && m_tilePreviewAspectCorrection) {
						m_selectedTilePreview = m_converter->GeneratePreview(m_selectedTileImage, m_tilePreviewAspectCorrection, m_tilePreviewScanlines);
						if (!m_selectedTilePreview) {
							AppConsole::AddLogF(AppConsole::LogLevel::Warning, "GeneratePreview failed for tile #%d, using original", m_selectedTileIndex);
							m_selectedTilePreview = m_selectedTileImage;
						}
					} else {
						m_selectedTilePreview = m_selectedTileImage;
					}
					m_cachedTilePreviewIndex = m_selectedTileIndex;
					m_cachedTilePreviewAspect = m_tilePreviewAspectCorrection;
					m_cachedTilePreviewScanlines = m_tilePreviewScanlines;
				}

				// Use cached preview image to create texture
				if (m_selectedTilePreview) {
					textureTilePreview = m_selectedTilePreview->GetTexture(Application::GetRenderer());
					if (!textureTilePreview) {
						AppConsole::AddLogF(AppConsole::LogLevel::Error, "Failed to create texture for tile #%d (size: %dx%d)", m_selectedTileIndex,
											m_selectedTilePreview->GetWidth(), m_selectedTilePreview->GetHeight());
					}
				}

				// Update right zoom state: texture size = display image size; logical size = original tile size
				if (textureTilePreview && m_selectedTilePreview) {
					m_extractionZoomStateRight.textureSize = ImVec2((float)m_selectedTilePreview->GetWidth(), (float)m_selectedTilePreview->GetHeight());
					m_extractionZoomStateRight.logicalSize = ImVec2((float)m_selectedTileImage->GetWidth(), (float)m_selectedTileImage->GetHeight());
					m_extractionZoomStateRight.name = m_name + " - Tile #" + std::to_string(m_selectedTileIndex);
					m_extractionZoomStateRight.showInfo = true;
					m_extractionZoomStateRight.showPixelGrid = true;
					m_extractionZoomStateRight.maintainAspectRatio = true;
				}
			}
			// Calculate tile position in the preview image to highlight it on the left panel
			if (m_converter) {
				auto convertedImage = m_converter->GetConverted(params);
				if (convertedImage && tileParams) {
					int imageWidth = convertedImage->GetWidth();
					int imageHeight = convertedImage->GetHeight();
					//
					// Calculate grid dimensions
					//
					int availableWidth = imageWidth - tileParams->OffsetX;
					int availableHeight = imageHeight - tileParams->OffsetY;
					if (availableWidth >= tileParams->TileWidth && availableHeight >= tileParams->TileHeight) {
						int tilesX = 1 + ((availableWidth - tileParams->TileWidth) / (tileParams->TileWidth + tileParams->PaddingX));
						//
						// Calculate tile position from index
						//
						int tileX = m_selectedTileIndex % tilesX;
						int tileY = m_selectedTileIndex / tilesX;
						//
						// Calculate pixel coordinates of the tile
						//
						float pixelX = (float)(tileParams->OffsetX + tileX * (tileParams->TileWidth + tileParams->PaddingX));
						float pixelY = (float)(tileParams->OffsetY + tileY * (tileParams->TileHeight + tileParams->PaddingY));
						//
						// Set selection rectangle on left zoom state (x, y, width, height in logical coordinates)
						//
						m_extractionZoomStateLeft.selection = ImVec4(pixelX, pixelY, (float)tileParams->TileWidth, (float)tileParams->TileHeight);
						m_extractionZoomStateLeft.selectionActive = true;
					}
				}
			}
		} else {
			//
			// Clear selection when no tile is selected
			//
			m_extractionZoomStateLeft.selectionActive = false;
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
		// Left area (tile viewer + tile list)
		//
		if (ImGui::BeginChild("TileViewerPanel", ImVec2(m_hSizeLeft, 0), false)) {
			ImVec2 leftAvail = ImGui::GetContentRegionAvail();
			m_extractionVSizeBottom = leftAvail.y - (m_extractionVSizeTop + Application::splitterThickness + ImGui::GetStyle().ItemSpacing.y);
			//
			// Vertical splitter: top (image viewer) | bottom (tile list)
			//
			ImGui::DrawSplitter(true, Application::splitterThickness, &m_extractionVSizeTop, &m_extractionVSizeBottom, vMinTop, vMinBottom);
			//
			// Top: image viewer
			//
			if (ImGui::BeginChild("ExtractionImageViewArea", ImVec2(-1, m_extractionVSizeTop), false)) {
				//
				// Configure image viewer widget using setters
				// Show preview controls in LEFT panel (preview image + controls)
				// Disable zoom/pan sync (different images: full preview vs selected tile)
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
				// Right panel: preview controls for selected tile (no conversion - uses GeneratePreview)
				//
				m_extractionViewer->SetPreviewControlsRight(&m_tilePreviewAspectCorrection, &m_tilePreviewScanlines, false);
				m_extractionViewer->SetPickingEnabled(false);
				m_extractionViewer->SetSyncZoomPan(false);
				m_extractionViewer->RenderDualViewer(texturePreview, textureTilePreview);
			}
			ImGui::EndChild();
			//
			// Jump the splitter
			//
			ImVec2 SplitterSpace = ImGui::GetCursorScreenPos();
			SplitterSpace.y += Application::splitterThickness;
			ImGui::SetCursorScreenPos(SplitterSpace);
			//
			// Bottom: tile list panel
			//
			if (ImGui::BeginChild("TileListPanel", ImVec2(-1, m_extractionVSizeBottom), true)) {
				//
				// Render tile list widget (pass image dimensions for grid calculation)
				//
				int imageWidth = 0;
				int imageHeight = 0;
				if (m_converter) {
					auto convertedImage = m_converter->GetConverted(params);
					if (convertedImage) {
						imageWidth = convertedImage->GetWidth();
						imageHeight = convertedImage->GetHeight();
					}
				}
				auto tileListResult = TileListWidget::Render(m_tileExtractor, tileParams, imageWidth, imageHeight);
				//
				// Handle tile selection
				//
				if (tileListResult.tileSelected) {
					//
					// Store the selected tile index to display in right panel
					//
					m_selectedTileIndex = tileListResult.selectedTileIndex;
					AppConsole::AddLogF(AppConsole::LogLevel::Info, "Tile selected: %d", m_selectedTileIndex);
				}
				//
				// Handle tile deletion/undeletion
				//
				if (tileListResult.tileDeleted) {
					//
					// Check if tile is currently deleted
					//
					auto it = std::find(tileParams->DeletedTiles.begin(), tileParams->DeletedTiles.end(), tileListResult.deletedTileIndex);
					bool wasDeleted = (it != tileParams->DeletedTiles.end());
					if (wasDeleted) {
						//
						// Undelete: remove from deleted list
						//
						tileParams->DeletedTiles.erase(it);
						AppConsole::AddLogF(AppConsole::LogLevel::Info, "Tile #%d restored", tileListResult.deletedTileIndex);
					} else {
						//
						// Delete: add to deleted list
						//
						tileParams->DeletedTiles.push_back(tileListResult.deletedTileIndex);
						AppConsole::AddLogF(AppConsole::LogLevel::Info, "Tile #%d deleted", tileListResult.deletedTileIndex);
					}
					//
					// Mark document and project as modified
					//
					SetModified(true);
					RetrodevLib::Project::MarkAsModified();
					//
					// Re-extract tiles to update the list
					//
					if (m_converter) {
						m_converter->Convert(params);
						auto convertedImage = m_converter->GetConverted(params);
						if (m_tileExtractor && convertedImage) {
							if (m_tileExtractor->Extract(convertedImage, tileParams)) {
								AppConsole::AddLogF(AppConsole::LogLevel::Info, "%d tiles available", m_tileExtractor->GetTileCount());
								//
								// Clear selection if deleted tile was selected
								//
								if (wasDeleted == false) {
									m_selectedTileIndex = -1;
								}
							}
						}
					}
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		ImGui::SameLine();
		//
		// Right area: tile extraction tooling panel
		//
		if (ImGui::BeginChild("TileExtractionTooling", ImVec2(m_hSizeRight, 0), true)) {
			//
			// Add left padding to prevent controls from being clipped at edge
			//
			ImGui::Indent(8.0f);
			//
			// 1. Tile extraction parameters widget
			//
			auto extractionResult = TileExtractionWidget::Render(tileParams, m_tileExtractor, m_converter, params);
			//
			// Handle parameter changes
			//
			if (extractionResult.parametersChanged) {
				SetModified(true);
				RetrodevLib::Project::MarkAsModified();
			}
			//
			// Clear deleted tiles if grid structure changed
			//
			if (extractionResult.gridStructureChanged && !tileParams->DeletedTiles.empty()) {
				tileParams->DeletedTiles.clear();
				AppConsole::AddLog(AppConsole::LogLevel::Info, "Deleted tiles list cleared due to grid structure change");
				SetModified(true);
				RetrodevLib::Project::MarkAsModified();
			}
			//
			// Handle extract button click
			//
			if (extractionResult.extractRequested) {
				if (m_converter) {
					m_converter->Convert(params);
					auto convertedImage = m_converter->GetConverted(params);
					if (m_tileExtractor && convertedImage) {
						if (m_tileExtractor->Extract(convertedImage, tileParams)) {
							AppConsole::AddLogF(AppConsole::LogLevel::Info, "Extracted %d tiles", m_tileExtractor->GetTileCount());
						} else {
							AppConsole::AddLog(AppConsole::LogLevel::Warning, "Failed to extract tiles");
						}
					}
				}
			}
			ImGui::Separator();
			//
			// 2. Export widget (script-driven export)
			//
			m_exportWidget.Render(RetrodevLib::ProjectBuildType::Tilemap, m_name, params, m_converter, m_tileExtractor, tileParams);
			//
			// Remove left padding
			//
			ImGui::Unindent(8.0f);
		}
		ImGui::EndChild();
	}
} // namespace RetrodevGui
