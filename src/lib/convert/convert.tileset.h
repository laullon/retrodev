// --------------------------------------------------------------------------------------------------------------
//
//
//
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
		// sourceImage: the tileset image to extract tiles from
		// params: tile extraction parameters (size, offset, padding)
		// Returns true if extraction succeeded, false otherwise
		//
		virtual bool Extract(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) = 0;
		//
		// Get the number of tiles that were extracted
		// Returns 0 if Extract() hasn't been called or failed
		//
		virtual int GetTileCount() const = 0;
		//
		// Get a specific tile by index
		// index: tile index (0 to GetTileCount()-1)
		// Returns nullptr if index is out of range or no tiles extracted
		//
		virtual std::shared_ptr<Image> GetTile(int index) const = 0;
	};
} // namespace RetrodevLib
