// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "cpc.sprites.h"
#include "cpc.bitmap.h"
#include <log/log.h>
#include <algorithm>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Constructor
	//
	CPCSpriteExtractor::CPCSpriteExtractor(IBitmapConverter* converter) : m_converter(converter) {}

	CPCSpriteExtractor::~CPCSpriteExtractor() {}
	//
	// Extract sprites from the source image
	//
	bool CPCSpriteExtractor::Extract(std::shared_ptr<Image> sourceImage, const SpriteExtractionParams* params) {
		//
		// Clear previous extraction results
		//
		m_sprites.clear();
		m_definitions.clear();
		//
		// Validate input
		//
		if (!sourceImage) {
			Log::Warning(LogChannel::General, "[CPC Sprites] Extract called with no source image.");
			return false;
		}
		if (!params) {
			Log::Warning(LogChannel::General, "[CPC Sprites] Extract called with null params.");
			return false;
		}
		if (params->Sprites.empty()) {
			return true;
		}
		Log::Info(LogChannel::General, "[CPC Sprites] Extracting %d sprite(s) from %dx%d image.",
			static_cast<int>(params->Sprites.size()), sourceImage->GetWidth(), sourceImage->GetHeight());
		//
		// Extract each sprite definition
		//
		for (size_t i = 0; i < params->Sprites.size(); i++) {
			const auto& spriteDef = params->Sprites[i];
			//
			// Validate sprite bounds
			//
			if (spriteDef.X < 0 || spriteDef.Y < 0 || spriteDef.Width <= 0 || spriteDef.Height <= 0) {
				Log::Warning(LogChannel::General, "[CPC Sprites] Sprite %d has invalid dimensions, skipping.", static_cast<int>(i));
				continue;
			}
			if (spriteDef.X + spriteDef.Width > sourceImage->GetWidth() || spriteDef.Y + spriteDef.Height > sourceImage->GetHeight()) {
				Log::Warning(LogChannel::General, "[CPC Sprites] Sprite %d is out of image bounds, skipping.", static_cast<int>(i));
				continue;
			}
			//
			// Create new image for this sprite
			//
			auto spriteImage = Image::ImageCreate(spriteDef.Width, spriteDef.Height);
			if (!spriteImage) {
				Log::Error(LogChannel::General, "[CPC Sprites] Failed to create image for sprite %d.", static_cast<int>(i));
				continue;
			}
			//
			// Copy pixels from source to sprite image
			//
			for (int y = 0; y < spriteDef.Height; y++) {
				for (int x = 0; x < spriteDef.Width; x++) {
					int srcX = spriteDef.X + x;
					int srcY = spriteDef.Y + y;
					RgbColor color = sourceImage->GetPixelColor(srcX, srcY);
					spriteImage->SetPixelColor(x, y, color);
				}
			}
			//
			// Store the extracted sprite and its definition
			//
			m_sprites.push_back(spriteImage);
			m_definitions.push_back(spriteDef);
		}
		Log::Info(LogChannel::General, "[CPC Sprites] Extraction complete: %d sprite(s) extracted.", static_cast<int>(m_sprites.size()));
		return true;
	}
	//
	// Get the number of sprites that were extracted
	//
	int CPCSpriteExtractor::GetSpriteCount() const {
		return static_cast<int>(m_sprites.size());
	}
	//
	// Get a specific sprite by index
	//
	std::shared_ptr<Image> CPCSpriteExtractor::GetSprite(int index) const {
		if (index < 0 || index >= static_cast<int>(m_sprites.size()))
			return nullptr;
		return m_sprites[index];
	}
	//
	// Get the definition of a specific sprite by index
	//
	const SpriteDefinition* CPCSpriteExtractor::GetSpriteDefinition(int index) const {
		if (index < 0 || index >= static_cast<int>(m_definitions.size()))
			return nullptr;
		return &m_definitions[index];
	}
} // namespace RetrodevLib::ConverterAmstradCPC
