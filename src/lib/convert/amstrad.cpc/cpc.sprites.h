// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.sprites.h>
#include <convert/convert.bitmap.h>
#include <memory>
#include <vector>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Forward declaration
	//
	class CPCBitmap;
	//
	// Amstrad CPC sprite extractor
	// Extracts individual sprites from an image based on sprite definitions
	// No conversion or processing, just direct pixel extraction
	//
	class CPCSpriteExtractor : public ISpriteExtractor {
	public:
		//
		// Constructor receives reference to parent converter
		// This allows querying conversion parameters when needed
		//
		CPCSpriteExtractor(IBitmapConverter* converter);
		~CPCSpriteExtractor() override;
		//
		// Extract sprites from the input image using the given parameters
		// Performs direct pixel-by-pixel copy from source to individual sprite images
		//
		bool Extract(std::shared_ptr<Image> sourceImage, const SpriteExtractionParams* params) override;
		//
		// Get the number of sprites that were extracted
		//
		int GetSpriteCount() const override;
		//
		// Get a specific sprite by index
		//
		std::shared_ptr<Image> GetSprite(int index) const override;
		//
		// Get the definition of a specific sprite by index
		//
		const SpriteDefinition* GetSpriteDefinition(int index) const override;

	private:
		//
		// Reference to parent converter (not owned)
		// Can be cast to CPCBitmap* to query CPC-specific parameters
		//
		[[maybe_unused]] IBitmapConverter* m_converter;
		//
		// Array of extracted sprite images
		//
		std::vector<std::shared_ptr<Image>> m_sprites;
		//
		// Array of sprite definitions (kept for querying)
		//
		std::vector<SpriteDefinition> m_definitions;
	};
} // namespace RetrodevLib::ConverterAmstradCPC
