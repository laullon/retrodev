// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC tileset converter -- grid slicing and pixel encoding.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "cpc.tileset.h"
#include "cpc.bitmap.h"
#include <assets/image/image.h>
#include <log/log.h>
#include <algorithm>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Constructor - receives reference to parent converter
	//
	CPCTileExtractor::CPCTileExtractor(IBitmapConverter* converter) : m_converter(converter) {}
	//
	// Destructor
	//
	CPCTileExtractor::~CPCTileExtractor() {
		tiles.clear();
	}
	//
	// Extract tiles from the input image using the given parameters
	// Performs direct pixel-by-pixel copy from source to individual tile images
	//
	bool CPCTileExtractor::Extract(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) {
		//
		// Clear any previously extracted tiles
		//
		tiles.clear();
		//
		// Validate input
		//
		if (!sourceImage || !params) {
			Log::Warning(LogChannel::General, "[CPC Tileset] Extract called with null image or params.");
			return false;
		}
		if (params->TileWidth <= 0 || params->TileHeight <= 0) {
			Log::Warning(LogChannel::General, "[CPC Tileset] Invalid tile dimensions (%dx%d).", params->TileWidth, params->TileHeight);
			return false;
		}
		//
		// Calculate number of tiles that can be extracted
		//
		int imageWidth = sourceImage->GetWidth();
		int imageHeight = sourceImage->GetHeight();
		//
		// Calculate available space after offset
		//
		int availableWidth = imageWidth - params->OffsetX;
		int availableHeight = imageHeight - params->OffsetY;
		//
		// Check if there's enough space for at least one tile
		//
		if (availableWidth < params->TileWidth || availableHeight < params->TileHeight) {
			Log::Warning(LogChannel::General, "[CPC Tileset] Image too small for a single tile (%dx%d tile in %dx%d available area).", params->TileWidth, params->TileHeight,
						 availableWidth, availableHeight);
			return false;
		}
		//
		// Calculate how many tiles fit in each dimension
		// Formula: (AvailableSpace - TileSize) / (TileSize + Padding) + 1
		//
		int tilesX = 1 + ((availableWidth - params->TileWidth) / (params->TileWidth + params->PaddingX));
		int tilesY = 1 + ((availableHeight - params->TileHeight) / (params->TileHeight + params->PaddingY));
		Log::Info(LogChannel::General, "[CPC Tileset] Extracting %dx%d grid of %dx%d tiles from %dx%d image.", tilesX, tilesY, params->TileWidth, params->TileHeight, imageWidth,
				  imageHeight);
		//
		// Reserve space for all tiles
		//
		tiles.reserve(tilesX * tilesY);
		//
		// Extract each tile
		//
		int absoluteTileIndex = 0;
		for (int tileY = 0; tileY < tilesY; tileY++) {
			for (int tileX = 0; tileX < tilesX; tileX++) {
				//
				// Check if this tile is marked for deletion (use absolute index)
				//
				bool isDeleted = std::find(params->DeletedTiles.begin(), params->DeletedTiles.end(), absoluteTileIndex) != params->DeletedTiles.end();
				absoluteTileIndex++;
				//
				// Skip deleted tiles
				//
				if (isDeleted)
					continue;
				//
				// Calculate source position for this tile
				//
				int srcX = params->OffsetX + tileX * (params->TileWidth + params->PaddingX);
				int srcY = params->OffsetY + tileY * (params->TileHeight + params->PaddingY);
				//
				// Create a new image for this tile
				//
				auto tileImage = Image::ImageCreate(params->TileWidth, params->TileHeight);
				if (!tileImage) {
					Log::Error(LogChannel::General, "[CPC Tileset] Failed to create image for tile (%d,%d).", tileX, tileY);
					continue;
				}
				//
				// Copy pixels from source to tile (direct copy, no processing)
				//
				for (int y = 0; y < params->TileHeight; y++) {
					for (int x = 0; x < params->TileWidth; x++) {
						//
						// Bounds check
						//
						int sourcePixelX = srcX + x;
						int sourcePixelY = srcY + y;
						if (sourcePixelX >= imageWidth || sourcePixelY >= imageHeight)
							continue;
						//
						// Copy pixel color
						//
						RgbColor color = sourceImage->GetPixelColor(sourcePixelX, sourcePixelY);
						tileImage->SetPixelColor(x, y, color);
					}
				}
				//
				// Add tile to the collection
				//
				tiles.push_back(tileImage);
			}
		}
		Log::Info(LogChannel::General, "[CPC Tileset] Extraction complete: %d tile(s) extracted.", static_cast<int>(tiles.size()));
		return !tiles.empty();
	}
	//
	// Extract all tiles ignoring the deleted list
	// Populates allTiles with every grid position regardless of deletion state
	//
	bool CPCTileExtractor::ExtractAll(std::shared_ptr<Image> sourceImage, const TileExtractionParams* params) {
		allTiles.clear();
		if (!sourceImage || !params)
			return false;
		if (params->TileWidth <= 0 || params->TileHeight <= 0)
			return false;
		int imageWidth = sourceImage->GetWidth();
		int imageHeight = sourceImage->GetHeight();
		int availableWidth = imageWidth - params->OffsetX;
		int availableHeight = imageHeight - params->OffsetY;
		if (availableWidth < params->TileWidth || availableHeight < params->TileHeight)
			return false;
		int tilesX = 1 + ((availableWidth - params->TileWidth) / (params->TileWidth + params->PaddingX));
		int tilesY = 1 + ((availableHeight - params->TileHeight) / (params->TileHeight + params->PaddingY));
		allTiles.reserve(tilesX * tilesY);
		for (int tileY = 0; tileY < tilesY; tileY++) {
			for (int tileX = 0; tileX < tilesX; tileX++) {
				int srcX = params->OffsetX + tileX * (params->TileWidth + params->PaddingX);
				int srcY = params->OffsetY + tileY * (params->TileHeight + params->PaddingY);
				auto tileImage = Image::ImageCreate(params->TileWidth, params->TileHeight);
				if (!tileImage) {
					allTiles.push_back(nullptr);
					continue;
				}
				for (int y = 0; y < params->TileHeight; y++) {
					for (int x = 0; x < params->TileWidth; x++) {
						int sourcePixelX = srcX + x;
						int sourcePixelY = srcY + y;
						if (sourcePixelX >= imageWidth || sourcePixelY >= imageHeight)
							continue;
						RgbColor color = sourceImage->GetPixelColor(sourcePixelX, sourcePixelY);
						tileImage->SetPixelColor(x, y, color);
					}
				}
				allTiles.push_back(tileImage);
			}
		}
		return !allTiles.empty();
	}
	//
	// Get the number of tiles that were extracted (excluding deleted)
	//
	int CPCTileExtractor::GetTileCount() const {
		return static_cast<int>(tiles.size());
	}
	//
	// Get a specific tile by index (excluding deleted)
	//
	std::shared_ptr<Image> CPCTileExtractor::GetTile(int index) const {
		if (index < 0 || index >= static_cast<int>(tiles.size()))
			return nullptr;
		return tiles[index];
	}
	//
	// Get the total count from the full (unfiltered) extraction
	//
	int CPCTileExtractor::GetTileAllCount() const {
		return static_cast<int>(allTiles.size());
	}
	//
	// Get a tile by absolute grid index from the full extraction
	//
	std::shared_ptr<Image> CPCTileExtractor::GetTileAll(int index) const {
		if (index < 0 || index >= static_cast<int>(allTiles.size()))
			return nullptr;
		return allTiles[index];
	}
}
