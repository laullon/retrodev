//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "tile.extraction.widget.h"

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
	//
	// Main render function for the tile extraction widget
	//
	TileExtractionWidgetResult TileExtractionWidget::Render(RetrodevLib::TileExtractionParams* tileParams, std::shared_ptr<RetrodevLib::ITileExtractor> tileExtractor,
															std::shared_ptr<RetrodevLib::IBitmapConverter> converter, RetrodevLib::GFXParams* params) {
		TileExtractionWidgetResult result;
		if (!tileParams)
			return result;
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
			ImGui::SetTooltip("When enabled, tile width and height are snapped to multiples\nof the platform's encoding unit (%d x %d pixels).\nThis ensures each tile dimension fits whole bytes\nin the target system's native pixel format.", m_constraintW, m_constraintH);
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
					ImGui::SetTooltip("Width %d is not a multiple of %d (encoding alignment).\nAdjust or press +/- to snap to a valid value.", tileParams->TileWidth, m_constraintW);
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
					ImGui::SetTooltip("Height %d is not a multiple of %d (encoding alignment).\nAdjust or press +/- to snap to a valid value.", tileParams->TileHeight, m_constraintH);
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
				ImGui::Text("Tiles Extracted: %d", tileCount);
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
			}
			ImGui::Separator();
			//
			// Extract button
			//
			if (ImGui::Button("Extract Tiles", ImVec2(-1, 0))) {
				result.extractRequested = true;
			}
		} else {
			m_extractionOpen = false;
		}
		result.parametersChanged = changed;
		result.gridStructureChanged = gridChanged;
		return result;
	}
} // namespace RetrodevGui
