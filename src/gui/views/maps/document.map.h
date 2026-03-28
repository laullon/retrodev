// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Map editor document -- tile map painting with layers and parallax.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <views/main.view.document.h>
#include <retrodev.gui.h>
#include <retrodev.lib.h>
#include <widgets/data.export.widget.h>
#include <string>
#include <vector>
#include <memory>

struct SDL_Texture;

namespace RetrodevGui {

	class DocumentMap : public DocumentView {
	public:
		DocumentMap(const std::string& name);
		~DocumentMap() override;
		//
		// Render the map document content
		//
		void Perform() override;
		//
		// Flush UI absolute data back to lib compact params before project save.
		//
		bool Save() override;
		//
		// Get the build type this document represents
		//
		RetrodevLib::ProjectBuildType GetBuildType() const override { return RetrodevLib::ProjectBuildType::Map; }

	private:
		//
		// Export widget -- script-driven export for this map
		//
		DataExportWidget m_exportWidget;
		//
		// Per-tileset loaded data (converter + extractor kept alive for tile texture access)
		//
		struct LoadedTileset {
			//
			// Name of the variant currently loaded (active variant of the slot)
			// Used to detect when the active variant changes and a reload is needed
			//
			std::string loadedVariantName;
			std::shared_ptr<RetrodevLib::Image> sourceImage;
			std::shared_ptr<RetrodevLib::IBitmapConverter> converter;
			std::shared_ptr<RetrodevLib::ITileExtractor> extractor;
			//
			// Sorted absolute indices of deleted tiles in this tileset's extraction grid.
			// Populated from TileExtractionParams::DeletedTiles after LoadTileset().
			// Used to skip deleted slots in the tile palette and to validate cell values.
			//
			std::vector<int> deletedTiles;
			bool loaded = false;
		};
		//
		// Loaded tileset data, parallel to mapParams.tilesetNames
		//
		std::vector<LoadedTileset> m_loadedTilesets;
		//
		// Selected tileset index (into m_loadedTilesets / mapParams.tilesetNames)
		//
		int m_selectedTilesetIdx = -1;
		//
		// Selected tile index within the selected tileset
		//
		int m_selectedTileIdx = -1;
		//
		// Canvas cell display size in screen pixels
		//
		float m_cellSize = 32.0f;
		//
		// Splitter sizes (left canvas | right tooling)
		//
		bool m_sizesInitialized = false;
		float m_hSizeLeft = 0.0f;
		float m_hSizeRight = 0.0f;
		//
		// Pending dimension inputs (staged before Apply is pressed)
		//
		int m_pendingWidth = 16;
		int m_pendingHeight = 16;
		//
		// Whether the viewable area overlay box is drawn on the canvas
		//
		bool m_showViewableArea = false;
		//
		// Whether the tile grid lines are drawn on the canvas
		//
		bool m_showGrid = true;
		//
		// Target system and mode selected for pixel aspect ratio preview
		// "Agnostic" means no correction is applied (square pixels, default)
		//
		std::string m_targetSystem = "Agnostic";
		std::string m_targetMode;
		//
		// Normalized pixel aspect scale factors derived from m_targetSystem/m_targetMode
		// hScale and vScale are used for canvas cell draw size only (not hit-testing)
		//
		float m_aspectHScale = 1.0f;
		float m_aspectVScale = 1.0f;
		//
		// Current scroll position as an integer step count (X horizontal, Y vertical).
		// Tile offset = stepCount * mapSpeed. Storing steps (not tiles) guarantees
		// every position is an exact multiple of mapSpeed with no floating-point drift.
		//
		int m_viewScrollX = 0;
		int m_viewScrollY = 0;
		//
		// Index of the layer currently being painted (editing layer)
		//
		int m_editingLayerIdx = 0;
		//
		// Tracks the last editing layer index so pending dimensions are re-synced on change
		//
		int m_lastEditingLayerIdx = -1;
		//
		// Inline rename buffer for the layer being edited, and the index it belongs to
		//
		char m_layerRenameBuffer[256] = {};
		int m_layerRenameIdx = -1;
		//
		// Selected group index (into params->groups); -1 = none selected
		//
		int m_selectedGroupIdx = -1;
		//
		// True while waiting for the user to drag a region on the canvas to create a group
		//
		bool m_groupCapturing = false;
		//
		// True while the capture drag is in progress (mouse button held after initial click)
		//
		bool m_groupCaptureDragging = false;
		//
		// Anchor cell column and row at the start of the capture drag
		//
		int m_groupCaptureStartCol = -1;
		int m_groupCaptureStartRow = -1;
		//
		// Current end cell column and row tracked during the capture drag
		//
		int m_groupCaptureCurCol = -1;
		int m_groupCaptureCurRow = -1;
		//
		// Inline rename buffer for the map name; synced to m_name each frame when they differ
		//
		char m_nameBuffer[256] = {};
		//
		// Inline rename buffer and the index of the group being renamed (-1 = none)
		//
		char m_groupRenameBuffer[256] = {};
		int m_groupRenameIdx = -1;
		//
		// UI-side absolute-index layer and group data.
		// The lib (MapParams) always stores compact indices.
		// On open: compact->absolute loaded here. On save: absolute->compact flushed to params.
		//
		bool m_absDataLoaded = false;
		std::vector<RetrodevLib::MapLayer> m_absLayers;
		std::vector<RetrodevLib::TileGroup> m_absGroups;
		//
		// Convert compact params->layers/groups into m_absLayers/m_absGroups (compact->absolute).
		//
		void LoadAbsData(RetrodevLib::MapParams* params);
		//
		// Convert m_absLayers/m_absGroups back into params->layers/groups (absolute->compact).
		//
		void FlushAbsData(RetrodevLib::MapParams* params);
		//
		// Synchronise m_loadedTilesets to match the given slot list,
		// reusing already-loaded entries and reloading when the active variant changes.
		// Warns and auto-fixes DeletedTiles discrepancies across variants in each slot.
		//
		void SyncLoadedTilesets(const std::vector<RetrodevLib::TilesetSlot>& slots);
		//
		// Load a single tileset entry: run conversion and tile extraction
		//
		void LoadTileset(LoadedTileset& ts);
		//
		// Return the SDL texture for a map cell word, or nullptr if unavailable
		//
		SDL_Texture* GetCellTexture(uint16_t cellVal);
		//
		// Encode a (tilesetIndex, tileIndex) pair into a map cell word
		// Encoding: top 4 bits = tilesetIndex + 1, bottom 12 bits = tileIndex; 0 means empty
		//
		static uint16_t EncodeMapTile(int tilesetIdx, int tileIdx);
		//
		// Decode the tileset index from a map cell word (0-based)
		//
		static int DecodeTilesetIdx(uint16_t val);
		//
		// Decode the tile index from a map cell word (0-based)
		//
		static int DecodeTileIdx(uint16_t val);
		//
		// Resize the layer data in-place, preserving existing tile data where it fits
		//
		static void ResizeMapData(RetrodevLib::MapLayer* layer, int newWidth, int newHeight);
		//
		// Insert an empty row at the top of the layer
		//
		static void AddRowTop(RetrodevLib::MapLayer* layer);
		//
		// Append an empty row at the bottom of the layer
		//
		static void AddRowBottom(RetrodevLib::MapLayer* layer);
		//
		// Insert an empty column at the left of every row
		//
		static void AddColumnLeft(RetrodevLib::MapLayer* layer);
		//
		// Append an empty column at the right of every row
		//
		static void AddColumnRight(RetrodevLib::MapLayer* layer);
		//
		// Remove the top row from the layer
		//
		static void RemoveRowTop(RetrodevLib::MapLayer* layer);
		//
		// Remove the bottom row from the layer
		//
		static void RemoveRowBottom(RetrodevLib::MapLayer* layer);
		//
		// Remove the leftmost column from every row
		//
		static void RemoveColumnLeft(RetrodevLib::MapLayer* layer);
		//
		// Remove the rightmost column from every row
		//
		static void RemoveColumnRight(RetrodevLib::MapLayer* layer);
		//
		// Render the toolbar strip above the map canvas (viewable area toggle, etc.)
		//
		void RenderCanvasToolbar(RetrodevLib::MapParams* params);
		//
		// Render the bottom scrollbar toolbar: custom H/V controls stepping by map speed
		//
		void RenderCanvasScrollbars(RetrodevLib::MapParams* params);
		//
		// Render the map painting canvas (left panel, no ImGui scroll)
		//
		void RenderMapCanvas(RetrodevLib::MapParams* params);
		//
		// Render the tooling panel (right panel): dimensions, tilesets, tile palette
		//
		void RenderToolingPanel(RetrodevLib::MapParams* params);
		//
		// Render the layers section: add, remove, rename, reorder and select map layers
		//
		void RenderLayersSection(RetrodevLib::MapParams* params);
		//
		// Render the groups section: add, remove, rename and select tile groups
		//
		void RenderGroupsSection(RetrodevLib::MapParams* params);
		//
		// Render a semi-transparent preview of a tile group at the given anchor cell position
		//
		void RenderGroupPreview(ImDrawList* drawList, ImVec2 canvasOrigin, const RetrodevLib::TileGroup& group, int anchorCol, int anchorRow);
	};

}
