// --------------------------------------------------------------------------------------------------------------
//
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <assets/palette/palette.params.h>
#include <assets/palette/palette.h>
#include <convert/convert.bitmap.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace RetrodevGui {

	class DocumentPalette : public DocumentView {
	public:
		DocumentPalette(const std::string& name);
		~DocumentPalette() override;
		//
		// Render the palette document content
		//
		void Perform() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Palette; }
		//
		// Called when any project build item changes so we can invalidate stale solution data
		//
		void OnProjectItemChanged(const std::string& itemName) override;

	private:
		//
		// Splitter state — main horizontal (left | right), right vertical (editor | solve),
		// and inner horizontal inside RenderRightPanel (participant list | palette preview)
		//
		float m_hSizeLeft = 0.0f;
		float m_hSizeRight = 0.0f;
		float m_vSizeTop = 0.0f;
		float m_vSizeBottom = 0.0f;
		float m_rightSizeList = 0.0f;
		float m_rightSizePal = 0.0f;
		bool m_sizesInitialized = false;
		//
		// Index of the currently selected zone (-1 = none)
		//
		int m_selectedZone = -1;
		//
		// Index of the currently selected participant within the selected zone (-1 = none)
		//
		int m_selectedParticipant = -1;
		//
		// Live IPaletteConverter for the current target — rebuilt when target changes
		//
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_converter;
		//
		// Target system/palette-type snapshot used to detect when a rebuild is needed
		//
		std::string m_lastTargetSystem;
		std::string m_lastTargetPaletteType;
		//
		// Render the left panel: target selection + zone list
		//
		void RenderLeftPanel(RetrodevLib::PaletteParams* params);
		//
		// Render the right panel: selected zone details + palette preview
		//
		void RenderRightPanel(RetrodevLib::PaletteParams* params);
		//
		// Ensure the converter matches the current target system/mode
		//
		void SyncConverter(RetrodevLib::PaletteParams* params);
		//
		// Add a new default zone to the palette
		//
		void AddZone(RetrodevLib::PaletteParams* params);
		//
		// Remove zone at index
		//
		void RemoveZone(RetrodevLib::PaletteParams* params, int index);
		//
		// Render the solve/validation panel at the bottom
		//
		void RenderSolvePanel(RetrodevLib::PaletteParams* params);
		//
		// Write solved palette assignments back into each participant's stored GFXParams
		//
		void ValidateSolution(RetrodevLib::PaletteParams* params);
		//
		// Render the level tag selector and participant thumbnails at the bottom of the left panel
		//
		void RenderThumbnails(RetrodevLib::PaletteParams* params);
		//
		// Last solve result — empty until the user triggers a solve
		//
		RetrodevLib::PaletteSolution m_solution;
		//
		// Whether a solve result is currently displayed
		//
		bool m_hasSolution = false;
		//
		// Currently selected zone index in the solution list (-1 = none)
		//
		int m_selectedSolutionZone = -1;
		//
		// Currently selected tag solution index within the selected zone (-1 = none, 0 = base)
		//
		int m_selectedSolutionTag = -1;
		//
		// Stable display converter and GFXParams for the selected solution.
		// Rebuilt whenever the selection changes. Both must outlive every frame that reads them.
		//
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_displayConverter;
		RetrodevLib::GFXParams m_displayGfx;
		//
		// Selection indices that were used to build m_displayConverter — used to detect changes
		//
		int m_lastDisplayZone = -1;
		int m_lastDisplayTag = -1;
		//
		// Key of the thumbnail selected in the left panel for solution preview ("" = none)
		// Format: "zoneName:buildItemType:buildItemName"
		//
		std::string m_selectedPreviewKey;
		//
		// Currently selected level tag for thumbnail filtering ("" = show all / Always / ScreenZone only)
		//
		std::string m_selectedLevelTag;
		//
		// Cached original-palette converter for the selected participant.
		// Rebuilt when the selected zone or participant changes.
		//
		std::shared_ptr<RetrodevLib::IBitmapConverter> m_originalConverter;
		RetrodevLib::GFXParams m_originalGfx;
		int m_lastOriginalZone = -1;
		int m_lastOriginalParticipant = -1;
		//
		// Per-thumbnail zoom/pan state — keyed by the preview key so each thumbnail
		// retains its own zoom level and pan offset across selection changes.
		//
		std::unordered_map<std::string, ImGui::ZoomableState> m_previewZoomStates;
	};

} // namespace RetrodevGui
