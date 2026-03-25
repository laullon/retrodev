// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Bitmap conversion interface and result type.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <convert/convert.palette.h>
#include <convert/convert.tileset.h>
#include <convert/convert.sprites.h>
#include <convert/convert.bitmap.params.h>
#include <string>
#include <vector>

namespace RetrodevLib {
	//
	// Interface to store and manipulate two different images
	// It contains source and destination
	// It should be implemented by every converter
	//
	class IBitmapConverter {
	public:
		virtual ~IBitmapConverter() = default;

		//
		// Used by the converter to return a list of target conversion modes
		// Since a converter can implement conversion for different video modes for that system
		//
		virtual std::vector<std::string> GetTargetModes() = 0;

		//
		// Used by the converter to return a predefined list of available palette types for the
		// current conversion target system / mode
		//
		virtual std::vector<std::string> GetTargetPalettes() = 0;

		//
		// Used by the converter to return a predefined list of available resolutions for the
		// current conversion target mode
		//
		virtual std::vector<std::string> GetTargetResolutions() = 0;

		//
		// Get the requested target resolution based on the resolution name and current params
		//
		virtual Image::Size GetTargetResolution(const std::string& resolution, const GFXParams* params) = 0;

		//
		// Returns the default resolution in pixels for the given target mode
		//
		virtual Image::Size GetDefaultResolution(int targetMode) = 0;

		//
		// Returns the minimum width/height multiple (in pixels) required by the platform's
		// pixel encoding to fit whole bytes. Width is typically > 1 (e.g. 2 for CPC Mode 0
		// where one byte encodes 2 pixels); height is usually 1 unless the platform packs
		// rows differently. Use this to constrain extracted sprite/tile dimensions.
		//
		virtual Image::Size GetEncodingAlignment(const GFXParams* params) = 0;

		//
		// Get the original image to be converted
		//
		virtual std::shared_ptr<Image> GetOriginal() = 0;

		//
		// Set the original image to be converted
		//
		virtual void SetOriginal(std::shared_ptr<Image> image) = 0;

		//
		// Returns the converted bitmap in a format suitable to be displayed for preview
		// If there is no bitmap yet and the Original one is fulfilled it triggers the conversion process with the
		// current parameters
		//
		virtual std::shared_ptr<Image> GetPreview(const GFXParams* params) = 0;
		//
		// Returns the converted bitmap at native resolution (before preview transformations)
		// This is the actual converted image in the target system's native format
		// Without aspect correction, scaling, or scanline effects applied
		//
		virtual std::shared_ptr<Image> GetConverted(const GFXParams* params) = 0;
		//
		// Returns the native width (pre-aspect-correction) of the converted image
		// This is the actual width in the target system (e.g., 160 for CPC Mode 0)
		// Used by the UI to display correct pixel grid and coordinates
		//
		virtual int GetNativeWidth(const GFXParams* params) = 0;
		//
		// Returns the native height (pre-aspect-correction) of the converted image
		// This is the actual height in the target system (e.g., 200 for CPC Mode 0)
		// Used by the UI to display correct pixel grid and coordinates
		//
		virtual int GetNativeHeight(const GFXParams* params) = 0;
		//
		// Returns the Palette object associated to this converter
		//
		virtual std::shared_ptr<IPaletteConverter> GetPalette() = 0;
		//
		// Returns the Tile Extractor associated to this converter
		// Created lazily on first call
		// The extractor receives a reference to this converter for querying conversion params
		//
		virtual std::shared_ptr<ITileExtractor> GetTileExtractor() = 0;
		//
		// Returns the Sprite Extractor associated to this converter
		// Created lazily on first call
		// The extractor receives a reference to this converter for querying conversion params
		//
		virtual std::shared_ptr<ISpriteExtractor> GetSpriteExtractor() = 0;
		//
		// Set preview display parameters (UI-only, not saved in project)
		// aspectCorrection: apply aspect ratio correction for display
		// scaleFactor: additional scaling factor (1-4)
		// scanlines: apply scanline effect
		//
		virtual void SetPreviewParams(bool aspectCorrection, int scaleFactor, bool scanlines) = 0;
		//
		// Generate a preview image from a given source image with preview parameters applied
		// This allows generating preview effects (aspect correction, scanlines) for arbitrary images
		// Used for sprite/tile preview where we want to apply preview effects to extracted images
		// sourceImage: the image to apply preview effects to
		// aspectCorrection: apply aspect ratio correction for display
		// scanlines: apply scanline effect
		// Returns a new image with preview effects applied
		//
		virtual std::shared_ptr<Image> GeneratePreview(std::shared_ptr<Image> sourceImage, bool aspectCorrection, bool scanlines) = 0;
		//
		// Get estimated size in bytes of the converted bitmap in native format
		// Returns total size including bitmap data and palette
		//
		virtual int GetEstimatedSize(const GFXParams* params) = 0;
		//
		// Perform the conversion with the given parameters
		//
		virtual void Convert(GFXParams* params) = 0;
	};

}
