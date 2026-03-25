// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Tile extraction widget -- grid parameters and extract controls.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "tile.extraction.widget.h"
#include <app/app.icons.mdi.h>

namespace RetrodevGui {
	//
	// Static member definitions
	//
	bool TileExtractionWidget::m_extractionOpen = true;
	bool TileExtractionWidget::m_constrainToGrid = false;
	int TileExtractionWidget::m_constraintW = 1;
	int TileExtractionWidget::m_constraintH = 1;
	int TileExtractionWidget::m_prevConstraintW = 1;
	int TileExtractionWidget::m_prevConstraintH = 1;
	int TileExtractionWidget::m_prevTileWidth = 0;
	int TileExtractionWidget::m_prevTileHeight = 0;
	float TileExtractionWidget::m_packBgR = 1.0f;
	float TileExtractionWidget::m_packBgG = 0.0f;
	float TileExtractionWidget::m_packBgB = 1.0f;
	int TileExtractionWidget::m_packMergeGap = 0;
	int TileExtractionWidget::m_packCellPad = 0;
	int TileExtractionWidget::m_packColumns = 0;
	bool TileExtractionWidget::m_packBgPickedFromImage = false;
	RetrodevLib::TileExtractionParams* TileExtractionWidget::m_lastTileParams = nullptr;
	//
	// Main render function for the tile extraction widget
	//
	TileExtractionWidgetResult TileExtractionWidget::Render(RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
															std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params) {
		TileExtractionWidgetResult result;
		if (!tileParams)
			return result;
		//
		// Sync pack UI state from tileParams whenever a new document is opened
		//
		if (tileParams != m_lastTileParams) {
			m_lastTileParams = tileParams;
			m_packBgR = tileParams->PackBgR;
			m_packBgG = tileParams->PackBgG;
			m_packBgB = tileParams->PackBgB;
			m_packMergeGap = tileParams->PackMergeGap;
			m_packCellPad = tileParams->PackCellPadding;
			m_packColumns = tileParams->PackColumns;
		}
		bool changed = false;
		bool gridChanged = false;
		//
		// Fetch encoding alignment constraint from converter
		//
		int newConstraintW = 1;
		int newConstraintH = 1;
		if (converter) {
			auto alignment = converter->GetEncodingAlignment(params);
			newConstraintW = (alignment.Width > 1) ? alignment.Width : 1;
			newConstraintH = (alignment.Height > 1) ? alignment.Height : 1;
		}
		if (newConstraintW != m_prevConstraintW || newConstraintH != m_prevConstraintH) {
			m_constrainToGrid = false;
			m_prevConstraintW = newConstraintW;
			m_prevConstraintH = newConstraintH;
		}
		m_constraintW = newConstraintW;
		m_constraintH = newConstraintH;
		bool hasConstraint = (m_constraintW > 1 || m_constraintH > 1);
		//
		// Constraint checkbox (disabled when no constraint applies)
		//
		ImGui::BeginDisabled(!hasConstraint);
		char constrainLabel[64];
		snprintf(constrainLabel, sizeof(constrainLabel), "Constrain to encoding alignment (%dx%d)", m_constraintW, m_constraintH);
		ImGui::AlignTextToFramePadding();
		ImGui::Checkbox(constrainLabel, &m_constrainToGrid);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
			ImGui::SetTooltip("When enabled, tile width and height are snapped to multiples\nof the platform's encoding unit (%d x %d pixels).\nThis ensures each tile dimension "
							  "fits whole bytes\nin the target system's native pixel format.",
							  m_constraintW, m_constraintH);
		}
		if (!hasConstraint)
			m_constrainToGrid = false;
		ImGui::Separator();
		//
		// Tile Extraction section
		//
		ImGui::SetNextItemOpen(m_extractionOpen, ImGuiCond_Once);
		if (ImGui::CollapsingHeader("Tile Extraction")) {
			m_extractionOpen = true;
			//
			// Tile size settings
			//
			ImGui::Text("Tile Size:");
			//
			// Highlight invalid values when constraint is active
			//
			bool widthInvalid = m_constrainToGrid && m_constraintW > 1 && (tileParams->TileWidth % m_constraintW) != 0;
			bool heightInvalid = m_constrainToGrid && m_constraintH > 1 && (tileParams->TileHeight % m_constraintH) != 0;
			if (widthInvalid)
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			if (ImGui::InputInt("Width##TileWidth", &tileParams->TileWidth)) {
				if (tileParams->TileWidth < 1)
					tileParams->TileWidth = 1;
				if (m_constrainToGrid && m_constraintW > 1) {
					int cW = m_constraintW;
					if (tileParams->TileWidth < m_prevTileWidth)
						tileParams->TileWidth = std::max(cW, (tileParams->TileWidth / cW) * cW);
					else
						tileParams->TileWidth = std::max(cW, ((tileParams->TileWidth + cW - 1) / cW) * cW);
				}
				m_prevTileWidth = tileParams->TileWidth;
				changed = true;
				gridChanged = true;
			}
			if (widthInvalid) {
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Width %d is not a multiple of %d (encoding alignment).\nAdjust or press +/- to snap to a valid value.", tileParams->TileWidth,
									  m_constraintW);
			}
			if (heightInvalid)
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
			if (ImGui::InputInt("Height##TileHeight", &tileParams->TileHeight)) {
				if (tileParams->TileHeight < 1)
					tileParams->TileHeight = 1;
				if (m_constrainToGrid && m_constraintH > 1) {
					int cH = m_constraintH;
					if (tileParams->TileHeight < m_prevTileHeight)
						tileParams->TileHeight = std::max(cH, (tileParams->TileHeight / cH) * cH);
					else
						tileParams->TileHeight = std::max(cH, ((tileParams->TileHeight + cH - 1) / cH) * cH);
				}
				m_prevTileHeight = tileParams->TileHeight;
				changed = true;
				gridChanged = true;
			}
			if (heightInvalid) {
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Height %d is not a multiple of %d (encoding alignment).\nAdjust or press +/- to snap to a valid value.", tileParams->TileHeight,
									  m_constraintH);
			}
			ImGui::Separator();
			//
			// Offset settings
			//
			ImGui::Text("Starting Offset:");
			if (ImGui::InputInt("Offset X##OffsetX", &tileParams->OffsetX)) {
				if (tileParams->OffsetX < 0)
					tileParams->OffsetX = 0;
				changed = true;
				gridChanged = true;
			}
			if (ImGui::InputInt("Offset Y##OffsetY", &tileParams->OffsetY)) {
				if (tileParams->OffsetY < 0)
					tileParams->OffsetY = 0;
				changed = true;
				gridChanged = true;
			}
			ImGui::Separator();
			//
			// Padding settings
			//
			ImGui::Text("Grid Padding:");
			if (ImGui::InputInt("Padding X##PaddingX", &tileParams->PaddingX)) {
				if (tileParams->PaddingX < 0)
					tileParams->PaddingX = 0;
				changed = true;
				gridChanged = true;
			}
			if (ImGui::InputInt("Padding Y##PaddingY", &tileParams->PaddingY)) {
				if (tileParams->PaddingY < 0)
					tileParams->PaddingY = 0;
				changed = true;
				gridChanged = true;
			}
			ImGui::Separator();
			//
			// Display tile extraction info
			//
			if (tileExtractor && converter) {
				int tileCount = tileExtractor->GetTileCount();
				int allCount = tileExtractor->GetTileAllCount();
				int deletedCount = allCount - tileCount;
				//
				// Calculate grid dimensions using native converted image
				//
				if (params) {
					auto convertedImage = converter->GetConverted(params);
					if (convertedImage) {
						int imageWidth = convertedImage->GetWidth();
						int imageHeight = convertedImage->GetHeight();
						int availableWidth = imageWidth - tileParams->OffsetX;
						int availableHeight = imageHeight - tileParams->OffsetY;
						if (availableWidth >= tileParams->TileWidth && availableHeight >= tileParams->TileHeight) {
							int tilesX = 1 + ((availableWidth - tileParams->TileWidth) / (tileParams->TileWidth + tileParams->PaddingX));
							int tilesY = 1 + ((availableHeight - tileParams->TileHeight) / (tileParams->TileHeight + tileParams->PaddingY));
							ImGui::Text("Grid Size: %dx%d", tilesX, tilesY);
						}
					}
				}
				ImGui::Text("Tiles Extracted: %d", allCount);
				if (deletedCount > 0)
					ImGui::TextColored(ImVec4(0.85f, 0.4f, 0.4f, 1.0f), "Deleted: %d  Active: %d", deletedCount, tileCount);
				else
					ImGui::Text("Active: %d", tileCount);
				//
				// Estimated raw size: per-tile byte count * active tile count
				// Byte count formula mirrors GetEstimatedSize but for tile dimensions
				//
				if (params && tileCount > 0) {
					int tileBytes = converter->GetEstimatedSize(params);
					int fullWidth = 0;
					int fullHeight = 0;
					if (auto img = converter->GetConverted(params)) {
						fullWidth = img->GetWidth();
						fullHeight = img->GetHeight();
					}
					if (fullWidth > 0 && fullHeight > 0) {
						//
						// Scale the full-image byte count down to a single tile, then multiply by active tiles
						//
						int totalPixels = fullWidth * fullHeight;
						int tilePixels = tileParams->TileWidth * tileParams->TileHeight;
						int singleTileBytes = (totalPixels > 0) ? (tileBytes * tilePixels / totalPixels) : 0;
						int totalBytes = singleTileBytes * tileCount;
						if (totalBytes < 1024)
							ImGui::Text("Est. Raw Size: %d bytes", totalBytes);
						else
							ImGui::Text("Est. Raw Size: %.1f KB", totalBytes / 1024.0f);
					}
				}
			}
			ImGui::Separator();
			//
			// Extract button
			//
			if (ImGui::Button("Extract Tiles", ImVec2(-1, 0))) {
				result.extractRequested = true;
			}
			//
			// Remove duplicates button -- enabled only when tiles have been extracted
			//
			bool hasTiles = tileExtractor && tileExtractor->GetTileCount() > 0;
			ImGui::BeginDisabled(!hasTiles);
			if (ImGui::Button("Remove Duplicates", ImVec2(-1, 0))) {
				result.removeDuplicatesRequested = true;
			}
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasTiles)
				ImGui::SetTooltip("Extract tiles first to enable duplicate detection.");
			//
			// Undelete all button -- enabled only when there are deleted tiles
			//
			bool hasDeleted = tileParams && !tileParams->DeletedTiles.empty();
			ImGui::BeginDisabled(!hasDeleted);
			if (ImGui::Button("Undelete All", ImVec2(-1, 0))) {
				result.undeleteAllRequested = true;
			}
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && !hasDeleted)
				ImGui::SetTooltip("No deleted tiles to restore.");
			ImGui::Separator();
			//
			// Pack-to-Grid section
			// Detects content regions in the converted image and rearranges them into
			// a regular grid so the tile extractor can process them with standard parameters.
			//
			ImGui::Text("Pack to Grid:");
			//
			// Enable toggle
			//
			bool packEnabledLocal = tileParams->PackEnabled;
			if (ImGui::Checkbox("Enabled##PackEnabled", &packEnabledLocal)) {
				tileParams->PackEnabled = packEnabledLocal;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_INFORMATION_OUTLINE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("When enabled, Pack-to-Grid runs automatically before tile extraction\n(required for the build pipeline).");
			//
			// Separator colour picker
			//
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Background:");
			ImGui::SameLine();
			float packColor[3] = {m_packBgR, m_packBgG, m_packBgB};
			if (ImGui::ColorEdit3("##PackBg", packColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
				m_packBgR = packColor[0];
				m_packBgG = packColor[1];
				m_packBgB = packColor[2];
				tileParams->PackBgR = m_packBgR;
				tileParams->PackBgG = m_packBgG;
				tileParams->PackBgB = m_packBgB;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_INFORMATION_OUTLINE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Background / separator colour to ignore during region detection.\nSet this to the solid colour surrounding your tile chunks.");
			//
			// Sample background from the converted image (pixel 0,0) as a quick shortcut
			//
			ImGui::SameLine();
			if (ImGui::Button("Sample##SampleBg"))
				m_packBgPickedFromImage = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Sample background colour from the top-left pixel of the converted image.");
			//
			// Merge gap
			//
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Merge gap:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::InputInt("##MergeGap", &m_packMergeGap)) {
				if (m_packMergeGap < 0)
					m_packMergeGap = 0;
				tileParams->PackMergeGap = m_packMergeGap;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_INFORMATION_OUTLINE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Row/column gap between adjacent regions that still belong to the same tile.\nIncrease if regions within a single tile are being split.");
			//
			// Cell padding
			//
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Cell padding:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::InputInt("##CellPad", &m_packCellPad)) {
				if (m_packCellPad < 0)
					m_packCellPad = 0;
				tileParams->PackCellPadding = m_packCellPad;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_INFORMATION_OUTLINE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Pixel gap between cells in the packed output image.");
			//
			// Grid columns (0 = auto)
			//
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Columns:");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
			if (ImGui::InputInt("##Columns", &m_packColumns)) {
				if (m_packColumns < 0)
					m_packColumns = 0;
				tileParams->PackColumns = m_packColumns;
				changed = true;
			}
			ImGui::SameLine();
			ImGui::TextDisabled(ICON_INFORMATION_OUTLINE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Number of columns in the packed grid. 0 = automatic (square root of region count).");
			ImGui::Separator();
			//
			// Pack button
			//
			if (ImGui::Button("Pack to Grid", ImVec2(-1, 0)))
				result.packToGridRequested = true;
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Detect content regions and rearrange them into a uniform grid.\nTile size and offsets are updated automatically after packing.");
			//
			// Expose the persistent pack settings so the document can read them
			//
			result.packEnabled = tileParams->PackEnabled;
			result.packBgR = m_packBgR;
			result.packBgG = m_packBgG;
			result.packBgB = m_packBgB;
			result.packMergeGap = m_packMergeGap;
			result.packCellPad = m_packCellPad;
			result.packColumns = m_packColumns;
			result.packSampleBg = m_packBgPickedFromImage;
			m_packBgPickedFromImage = false;
		} else {
			m_extractionOpen = false;
		}
		result.parametersChanged = changed;
		result.gridStructureChanged = gridChanged;
		return result;
	}
}
