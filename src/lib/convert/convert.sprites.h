// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <convert/convert.sprites.params.h>
#include <memory>
#include <vector>

namespace RetrodevLib {
	//
	// Forward declaration
	//
	class IBitmapConverter;
	//
	// Sprite extractor interface
	// Extracts individual sprites from an image based on sprite definitions
	//
	class ISpriteExtractor {
	public:
		virtual ~ISpriteExtractor() = default;
		//
		// Extract sprites from the input image using the given parameters
		// sourceImage: the image to extract sprites from
		// params: sprite extraction parameters (sprite definitions)
		// Returns true if extraction succeeded, false otherwise
		//
		virtual bool Extract(std::shared_ptr<Image> sourceImage, const SpriteExtractionParams* params) = 0;
		//
		// Get the number of sprites that were extracted
		// Returns 0 if Extract() hasn't been called or failed
		//
		virtual int GetSpriteCount() const = 0;
		//
		// Get a specific sprite by index
		// index: sprite index (0 to GetSpriteCount()-1)
		// Returns nullptr if index is out of range or no sprites extracted
		//
		virtual std::shared_ptr<Image> GetSprite(int index) const = 0;
		//
		// Get the definition of a specific sprite by index
		// index: sprite index (0 to GetSpriteCount()-1)
		// Returns nullptr if index is out of range
		//
		virtual const SpriteDefinition* GetSpriteDefinition(int index) const = 0;
	};
} // namespace RetrodevLib
