// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Tileset conversion interface and result type.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <convert/convert.tileset.params.h>
#include <memory>
#include <vector>

namespace RetrodevLib {
	//
	// Forward declaration
	//
	class IBitmapConverter;
	//
	// Tile extractor interface
	// Extracts individual tiles from a tileset image based on extraction parameters
	//
	class ITileExtractor {
	public:
		virtual ~ITileExtractor() = default;
		//
		// Extract tiles from the input image using the given parameters
		// Deleted tiles (per params->DeletedTiles) are skipped
		// sourceImage: the tileset image to extract tiles from
		// params: tile extraction parameters (size, offset, padding)
		// Returns true if extraction succeeded, false otherwise
		//
		virtual bool Extract(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) = 0;
		//
		// Extract all tiles ignoring the deleted list
		// Used to populate display images for deleted tile slots
		// Returns true if extraction succeeded, false otherwise
		//
		virtual bool ExtractAll(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) = 0;
		//
		// Get the number of tiles that were extracted (excluding deleted)
		// Returns 0 if Extract() hasn't been called or failed
		//
		virtual int GetTileCount() const = 0;
		//
		// Get a specific tile by index (excluding deleted)
		// index: tile index (0 to GetTileCount()-1)
		// Returns nullptr if index is out of range or no tiles extracted
		//
		virtual std::shared_ptr<Image> GetTile(int index) const = 0;
		//
		// Get the total number of tiles extracted by ExtractAll (including deleted positions)
		// Returns 0 if ExtractAll() hasn't been called
		//
		virtual int GetTileAllCount() const = 0;
		//
		// Get a tile by absolute grid index from the full (unfiltered) extraction
		// index: absolute grid index (0 to GetTileAllCount()-1)
		// Returns nullptr if index is out of range or ExtractAll() hasn't been called
		//
		virtual std::shared_ptr<Image> GetTileAll(int index) const = 0;
	};
}
