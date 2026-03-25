// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC bitmap converter -- pixel encoding for all three video modes.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.bitmap.h>
#include <convert/amstrad.cpc/amstrad.cpc.h>
#include <convert/amstrad.cpc/cpc.palette.h>
#include <convert/convert.bitmap.params.h>
#include <string>
#include <vector>

namespace RetrodevLib::ConverterAmstradCPC {
	//
	// Implements a converter for CPC Graphics
	//
	class CPCBitmap : public IBitmapConverter {
	public:
		CPCBitmap(const std::string& mode);
		CPCBitmap();
		~CPCBitmap() override;
		//
		// Interface implementation
		//
		//
		// Used by the converter to return a list of target conversion modes
		// Since a converter can implement conversion for different video modes for that system
		//
		std::vector<std::string> GetTargetModes() override;
		//
		// Used by the converter to return a predefined list of available resolutions for the
		// current conversion target mode
		//
		std::vector<std::string> GetTargetResolutions() override;
		//
		// Used by the converter to return a list of available palette types for the system
		//
		std::vector<std::string> GetTargetPalettes() override;
		//
		// Get the requested target resolution based on the resolution name and current params
		//
		Image::Size GetTargetResolution(const std::string& resolution, const GFXParams* params) override;
		//
		// Returns the default resolution in pixels for the given target mode
		//
		Image::Size GetDefaultResolution(int targetMode) override;
		//
		// Returns the encoding alignment for the given parameters
		// Mode 0: 1 byte = 2 pixels  -> width must be multiple of 2
		// Mode 1: 1 byte = 4 pixels  -> width must be multiple of 4
		// Mode 2: 1 byte = 8 pixels  -> width must be multiple of 8
		// Height is always 1 (no row-packing constraint on CPC)
		//
		Image::Size GetEncodingAlignment(const GFXParams* params) override;
		//
		// Get the original bitmap to be converted
		//
		std::shared_ptr<Image> GetOriginal() override;
		//
		// Set the original bitmap to be converted
		//
		void SetOriginal(std::shared_ptr<Image> image) override;
		//
		// Returns the converted bitmap in a format suitable to be displayed for preview
		// If there is no bitmap yet and the Original one is fulfilled it triggers the conversion process with the
		// current parameters
		//
		std::shared_ptr<Image> GetPreview(const GFXParams* params) override;
		//
		// Returns the converted bitmap at native resolution (before preview transformations)
		// This is the quantized/dithered CPC image at native resolution
		//
		std::shared_ptr<Image> GetConverted(const GFXParams* params) override;
		//
		// Returns the native width (pre-aspect-correction)
		// This is the actual CPC width before aspect correction scaling
		//
		int GetNativeWidth(const GFXParams* params) override;
		//
		// Returns the native height (pre-aspect-correction)
		// This is the actual CPC height before aspect correction scaling
		//
		int GetNativeHeight(const GFXParams* params) override;
		//
		// Returns the Palette object associated to this converter
		//
		std::shared_ptr<IPaletteConverter> GetPalette() override;
		//
		// Returns the Tile Extractor associated to this converter
		// Created lazily on first call
		//
		std::shared_ptr<ITileExtractor> GetTileExtractor() override;
		//
		// Returns the Sprite Extractor associated to this converter
		// Created lazily on first call
		//
		std::shared_ptr<ISpriteExtractor> GetSpriteExtractor() override;
		//
		// Set preview display parameters (UI-only, not saved in project)
		//
		void SetPreviewParams(bool aspectCorrection, int scaleFactor, bool scanlines) override;
		//
		// Generate a preview image from a given source image with preview parameters applied
		//
		std::shared_ptr<Image> GeneratePreview(std::shared_ptr<Image> sourceImage, bool aspectCorrection, bool scanlines) override;
		//
		// Get estimated size in bytes of the converted bitmap
		// Includes bitmap data + palette data
		//
		int GetEstimatedSize(const GFXParams* params) override;
		//
		// Implements the main function for the conversion on Amstrad CPC (464,472,664,6128)
		//
		void Convert(GFXParams* params) override;

	private:
		//
		// Original Image to be converted
		//
		std::shared_ptr<Image> srcImage;
		//
		// Converted image at native resolution (before preview transformations)
		// This is the quantized/dithered image at CPC native resolution
		// Before aspect correction, scaling, or scanlines
		//
		std::shared_ptr<Image> convertedImage;
		//
		// Preview image (converted + aspect correction + scaling + scanlines)
		// This is what's displayed in the UI
		//
		std::shared_ptr<Image> previewImage;
		//
		// Native resolution (before aspect correction)
		// Stored during conversion for UI display
		//
		int nativeWidth;
		int nativeHeight;
		//
		// Preview display parameters (UI-only)
		//
		bool previewAspectCorrection = true;
		int previewScaleFactor = 1;
		bool previewScanlines = false;

		//
		// Palette object
		//
		std::shared_ptr<CPCPalette> palette;
		//
		// Tile extractor (created lazily)
		//
		std::shared_ptr<ITileExtractor> tileExtractor;
		//
		// Sprite extractor (created lazily)
		//
		std::shared_ptr<ISpriteExtractor> spriteExtractor;
	};

}
