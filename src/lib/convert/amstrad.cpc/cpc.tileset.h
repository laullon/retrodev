// --------------------------------------------------------------------------------------------------------------
//
//
//
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
		// Get the number of tiles that were extracted
		//
		int GetTileCount() const override;
		//
		// Get a specific tile by index
		//
		std::shared_ptr<Image> GetTile(int index) const override;

	private:
		//
		// Reference to parent converter (not owned)
		// Can be cast to CPCBitmap* to query CPC-specific parameters
		//
		[[maybe_unused]] IBitmapConverter* m_converter;
		//
		// Array of extracted tile images
		//
		std::vector<std::shared_ptr<Image>> tiles;
	};
} // namespace RetrodevLib::ConverterAmstradCPC
