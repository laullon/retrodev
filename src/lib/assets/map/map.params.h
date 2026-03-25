// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Map asset parameters -- layer, tileset slot and cell data.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace RetrodevLib {

	//
	// A tileset slot groups one or more project tileset build-item names as variants.
	// The slot index is what gets encoded in the map cell data; switching activeVariant
	// changes which tileset is displayed for that slot without altering the map data.
	//
	struct TilesetSlot {
		//
		// Project tileset build-item names for this slot (at least one entry)
		//
		std::vector<std::string> variants;
		//
		// Index into variants that is currently active (displayed on the map)
		//
		int activeVariant = 0;
	};
	//
	// A named group of tiles captured from a rectangular region of the map
	// Groups can be stamped onto the map as a single unit
	//
	struct TileGroup {
		std::string name;
		//
		// Dimensions of the captured region in tiles
		//
		int width = 0;
		int height = 0;
		//
		// Tile data: width * height cell words (same encoding as map data)
		//
		std::vector<uint16_t> tiles;
	};
	//
	// A single layer in a map: owns its own tile matrix, dimensions and scroll speed.
	// Multiple layers can coexist in one map for parallax or foreground/background effects.
	//
	struct MapLayer {
		//
		// Display name shown in the Layers panel
		//
		std::string name;
		//
		// Layer dimensions in tiles
		//
		int width = 16;
		int height = 16;
		//
		// Scroll speed in tiles per camera step (1.0 = one tile per step).
		// Values < 1.0 create parallax (slower than camera); >= viewWidth/viewHeight means fixed/room.
		//
		float mapSpeed = 1.0f;
		//
		// Position offset in tiles (fractional values allowed).
		// Applied to the layer origin independently of scroll, allowing layers to sit at
		// different screen positions (e.g. offsetY = 0.5 shifts the layer down by half a tile).
		//
		float offsetX = 0.0f;
		float offsetY = 0.0f;
		//
		// Whether this layer is rendered on the canvas
		//
		bool visible = true;
		//
		// Tile matrix data: width * height entries.
		// 0 = empty cell.
		// Non-zero: top 4 bits = slotIndex + 1 (1-15), bottom 12 bits = tileIndex (0-4095).
		//
		std::vector<uint16_t> data;
	};
	//
	// Parameters for a map build item.
	// Tilesets and groups are shared across all layers; each layer owns its own tile matrix.
	//
	struct MapParams {
		//
		// Viewable area size in tiles: the portion of the map visible on screen at once
		//
		int viewWidth = 16;
		int viewHeight = 12;
		//
		// Tileset slots, each holding one or more variant names (project tileset build items).
		// The slot index is encoded in the cell data; activeVariant selects which variant renders.
		//
		std::vector<TilesetSlot> tilesets;
		//
		// Named tile groups captured from map regions
		//
		std::vector<TileGroup> groups;
		//
		// Map layers, rendered bottom to top.  At least one layer is required for painting.
		//
		std::vector<MapLayer> layers;
	};

}
