// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC tileset converter -- grid slicing and pixel encoding.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.tileset.h>
#include <convert/convert.bitmap.h>
#include <memory>
#include <vector>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Forward declaration
	//
	class CPCBitmap;
	//
	// Amstrad CPC tile extractor
	// Extracts individual tiles from a tileset image
	// No conversion or processing, just direct pixel extraction
	//
	class CPCTileExtractor : public ITileExtractor {
	public:
		//
		// Constructor receives reference to parent converter
		// This allows querying conversion parameters when needed
		//
		CPCTileExtractor(IBitmapConverter* converter);
		~CPCTileExtractor() override;
		//
		// Extract tiles from the input image using the given parameters
		// Performs direct pixel-by-pixel copy from source to individual tile images
		//
		bool Extract(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) override;
		//
		// Extract all tiles ignoring the deleted list
		//
		bool ExtractAll(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) override;
		//
		// Get the number of tiles that were extracted (excluding deleted)
		//
		int GetTileCount() const override;
		//
		// Get a specific tile by index (excluding deleted)
		//
		std::shared_ptr<Image> GetTile(int index) const override;
		//
		// Get the total count from the full (unfiltered) extraction
		//
		int GetTileAllCount() const override;
		//
		// Get a tile by absolute grid index from the full extraction
		//
		std::shared_ptr<Image> GetTileAll(int index) const override;

	private:
		//
		// Reference to parent converter (not owned)
		// Can be cast to CPCBitmap* to query CPC-specific parameters
		//
		[[maybe_unused]] IBitmapConverter* m_converter;
		//
		// Array of extracted tile images (deleted tiles excluded)
		//
		std::vector<std::shared_ptr<Image>> tiles;
		//
		// Array of all tile images including deleted positions (populated by ExtractAll)
		//
		std::vector<std::shared_ptr<Image>> allTiles;
	};
}
