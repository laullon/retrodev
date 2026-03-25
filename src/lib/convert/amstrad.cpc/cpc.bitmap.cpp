// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC bitmap converter -- pixel encoding for all three video modes.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "cpc.bitmap.h"
#include "cpc.palette.h"
#include <retrodev.lib.h>
#include "cpc.tileset.h"
#include "cpc.sprites.h"
#include <assets/image/image.h>
#include <convert/convert.bitmap.params.h>
#include <process/image/resize.h>
#include <process/image/quantization.h>
#include <process/image/color.correction.h>
#include <system/amstrad.cpc/devices/cpc.screen.h>
#include <log/log.h>

namespace RetrodevLib::ConverterAmstradCPC {

	//
	// Constructor with mode parameter
	//
	CPCBitmap::CPCBitmap(const std::string& mode) : srcImage(nullptr), nativeWidth(0), nativeHeight(0) {
		UNUSED(mode);
		palette = std::make_shared<CPCPalette>();
	}
	//
	// Default constructor
	//
	CPCBitmap::CPCBitmap() : srcImage(nullptr), nativeWidth(0), nativeHeight(0) {
		palette = std::make_shared<CPCPalette>();
	}
	//
	// Destructor
	//
	CPCBitmap::~CPCBitmap() {
		srcImage = nullptr;
		palette = nullptr;
	}
	//
	// Used by the converter to return a list of target conversion modes
	// Since a converter can implement conversion for different video modes for that system
	//
	std::vector<std::string> CPCBitmap::GetTargetModes() {
		return CPCModesList;
	}
	//
	// Used by the converter to return a predefined list of available resolutions for the
	// current conversion target mode
	//
	std::vector<std::string> CPCBitmap::GetTargetResolutions() {
		return CPCResolutionsList;
	}
	//
	// Used by the converter to return a list of available palette types for the system
	//
	std::vector<std::string> CPCBitmap::GetTargetPalettes() {
		return CPCPaletteTypesList;
	}
	//
	// Get the requested target resolution
	//
	Image::Size CPCBitmap::GetTargetResolution(const std::string& resolution, const GFXParams* params) {
		if (params == nullptr)
			return Image::Size{0, 0};
		//
		// Check if resolution matches predefined values
		//
		if (resolution == CPCResolutions::Custom)
			return Image::Size{params->RParams.TargetWidth, params->RParams.TargetHeight};
		//
		// Normal / Overscan: return corresponding resolution depending on the mode
		//
		bool isOverscan = (resolution == CPCResolutions::Overscan);
		if (params->SParams.TargetMode == CPCModes::Mode0)
			return isOverscan ? Image::Size{192, 272} : Image::Size{160, 200};
		else if (params->SParams.TargetMode == CPCModes::Mode1)
			return isOverscan ? Image::Size{384, 272} : Image::Size{320, 200};
		else if (params->SParams.TargetMode == CPCModes::Mode2)
			return isOverscan ? Image::Size{768, 272} : Image::Size{640, 200};
		return Image::Size{params->RParams.TargetWidth, params->RParams.TargetHeight};
	}
	//
	// Returns the default resolution in pixels for the given target mode
	//
	Image::Size CPCBitmap::GetDefaultResolution(int targetMode) {
		//
		// Deprecated: kept for compatibility, use string-based mode instead
		//
		if (targetMode == 0)
			return Image::Size{160, 200};
		if (targetMode == 1)
			return Image::Size{320, 200};
		if (targetMode == 2)
			return Image::Size{640, 200};
		return Image::Size{160, 200};
	}
	//
	// Returns the encoding alignment for the given parameters
	// Mode 0: 1 byte = 2 pixels  -> width must be multiple of 2
	// Mode 1: 1 byte = 4 pixels  -> width must be multiple of 4
	// Mode 2: 1 byte = 8 pixels  -> width must be multiple of 8
	// Height is always 1 (no row-packing constraint on CPC)
	//
	Image::Size CPCBitmap::GetEncodingAlignment(const GFXParams* params) {
		if (params == nullptr)
			return Image::Size{1, 1};
		if (params->SParams.TargetMode == CPCModes::Mode0)
			return Image::Size{2, 1};
		else if (params->SParams.TargetMode == CPCModes::Mode1)
			return Image::Size{4, 1};
		else if (params->SParams.TargetMode == CPCModes::Mode2)
			return Image::Size{8, 1};
		return Image::Size{1, 1};
	}
	//
	// Original source image to be converted
	//
	std::shared_ptr<Image> CPCBitmap::GetOriginal() {
		return srcImage;
	}
	//
	// Set the original image
	//
	void CPCBitmap::SetOriginal(std::shared_ptr<Image> image) {
		srcImage = image;
	}

	//
	// It returns the converted bitmap but in a format suitable to be painted in
	// this actual screen. It triggers the conversion also.
	//
	std::shared_ptr<Image> CPCBitmap::GetPreview(const GFXParams* params) {
		UNUSED(params);
		return previewImage;
	}
	//
	// Returns the converted bitmap at native resolution (before preview transformations)
	// This is the quantized/dithered CPC image without aspect correction
	//
	std::shared_ptr<Image> CPCBitmap::GetConverted(const GFXParams* params) {
		UNUSED(params);
		return convertedImage;
	}

	//
	// Returns the native width (pre-aspect-correction)
	// This is the actual CPC width before scaling for display
	//
	int CPCBitmap::GetNativeWidth(const GFXParams* params) {
		UNUSED(params);
		return nativeWidth;
	}

	//
	// Returns the native height (pre-aspect-correction)
	// This is the actual CPC height before scaling for display
	//
	int CPCBitmap::GetNativeHeight(const GFXParams* params) {
		UNUSED(params);
		return nativeHeight;
	}

	//
	// Returns the inner palette object
	//
	std::shared_ptr<IPaletteConverter> CPCBitmap::GetPalette() {
		if (palette == nullptr)
			palette = std::make_shared<CPCPalette>();
		return palette;
	}
	//
	// Returns the tile extractor (lazy initialization)
	// Passes 'this' converter reference so extractor can query conversion parameters
	//
	std::shared_ptr<ITileExtractor> CPCBitmap::GetTileExtractor() {
		if (tileExtractor == nullptr)
			tileExtractor = std::make_shared<CPCTileExtractor>(this);
		return tileExtractor;
	}
	//
	// Returns the sprite extractor (lazy initialization)
	// Passes 'this' converter reference so extractor can query conversion parameters
	//
	std::shared_ptr<ISpriteExtractor> CPCBitmap::GetSpriteExtractor() {
		if (spriteExtractor == nullptr)
			spriteExtractor = std::make_shared<CPCSpriteExtractor>(this);
		return spriteExtractor;
	}
	//
	// Set preview display parameters (UI-only, not saved in project)
	//
	void CPCBitmap::SetPreviewParams(bool aspectCorrection, int scaleFactor, bool scanlines) {
		previewAspectCorrection = aspectCorrection;
		previewScaleFactor = scaleFactor;
		previewScanlines = scanlines;
	}
	//
	// Generate a preview image from a given source image with preview parameters applied
	// This is used for sprite/tile preview to apply aspect correction and scanlines
	// without affecting the main conversion
	//
	std::shared_ptr<Image> CPCBitmap::GeneratePreview(std::shared_ptr<Image> sourceImage, bool aspectCorrection, bool scanlines) {
		if (!sourceImage)
			return nullptr;
		//
		// If aspect correction is disabled, return source image as-is
		// Scanlines are optional and only work with aspect correction
		//
		if (!aspectCorrection)
			return sourceImage;
		//
		// Apply aspect correction using existing screen rendering pipeline
		//
		CPCScreen::ScalingParams screenParams;
		screenParams.Mode = CPCModes::Mode0;
		screenParams.ScaleFactor = 1;
		screenParams.Scanlines = scanlines;
		screenParams.ScanlineIntensity = 0.3f;
		//
		// Apply aspect correction
		//
		auto previewImage = CPCScreen::ApplyAspectCorrection(sourceImage, screenParams);
		//
		// If aspect correction failed, fall back to original
		//
		return previewImage ? previewImage : sourceImage;
	}
	//
	// Get estimated size in bytes of the converted bitmap
	// CPC Mode 0: 4 pixels per byte (16 colors)
	// CPC Mode 1: 4 pixels per byte (4 colors)
	// CPC Mode 2: 8 pixels per byte (2 colors)
	//
	// In fact, in a CPC a scaline always occupies the same amount of bytes regardless of the number of colors
	// But in our case we work with pixels and not bytes.
	//
	// This could be troublesome for modes like EGX which mixes modes but
	// we will add the special cases (threat the image width for the larger one)
	// For example, an image alternating mode 0 and mode 1 will have 320 pixels per line but
	// in the conversion step we will just double the pixels in the lines for mode 0
	//
	int CPCBitmap::GetEstimatedSize(const GFXParams* params) {
		if (!params)
			return 0;
		int width = params->RParams.TargetWidth;
		int height = params->RParams.TargetHeight;
		int bitmapSize = 0;
		//
		// Calculate bitmap data size based on CPC mode
		//
		if (params->SParams.TargetMode == CPCModes::Mode0) {
			//
			// Mode 0: 160x200, 16 colors, 4 pixels per byte
			//
			bitmapSize = (width / 2) * height;
		} else if (params->SParams.TargetMode == CPCModes::Mode1) {
			//
			// Mode 1: 320x200, 4 colors, 4 pixels per byte
			//
			bitmapSize = (width / 4) * height;
		} else if (params->SParams.TargetMode == CPCModes::Mode2) {
			//
			// Mode 2: 640x200, 2 colors, 8 pixels per byte
			//
			bitmapSize = (width / 8) * height;
		}
		return bitmapSize;
	}
	//
	// Implements the main function for the conversion on Amstrad CPC (464,472,664,6128)
	//
	void CPCBitmap::Convert(GFXParams* params) {
		if (params == nullptr || palette == nullptr) {
			Log::Error("CPCBitmap::Convert called with null params or palette");
			return;
		}
		//
		// Sanity check: We dont have source image. Do nothing
		//
		if (srcImage == nullptr) {
			Log::Warning("CPCBitmap::Convert called with no source image");
			return;
		}
		//
		// Not initialized yet
		//
		if (params->RParams.TargetWidth == 0 || params->RParams.TargetHeight == 0) {
			Log::Warning("CPCBitmap::Convert called with zero target resolution");
			return;
		}
		Log::Info(LogChannel::General, "[CPC] Converting: mode=%s palette=%s target=%dx%d", params->SParams.TargetMode.c_str(), params->SParams.PaletteType.c_str(),
				  params->RParams.TargetWidth, params->RParams.TargetHeight);
		//
		// Set target mode & palette
		//
		palette->SetCPCMode(params->SParams.TargetMode);
		//
		// Set pointers to lock/enable arrays from params (no copying, direct reference)
		// This must be done BEFORE SetPaletteType to preserve locked pen colors
		//
		std::shared_ptr<CPCPalette> cpcPalette = std::static_pointer_cast<CPCPalette>(palette);
		//
		// Disable the transparent pen slot so the quantizer never assigns a color to it.
		// Save and restore around the full quantization pass since SetLockEnableArrays
		// stores a live pointer to params->SParams.PaletteEnabled.
		//
		int transPenSlot = params->RParams.TransparentPen;
		bool savedTransPenEnabled = true;
		if (transPenSlot >= 0) {
			if ((int)params->SParams.PaletteEnabled.size() <= transPenSlot)
				params->SParams.PaletteEnabled.resize(transPenSlot + 1, true);
			savedTransPenEnabled = params->SParams.PaletteEnabled[transPenSlot];
			params->SParams.PaletteEnabled[transPenSlot] = false;
		}
		if (cpcPalette) {
			cpcPalette->SetLockEnableArrays(params->SParams.PaletteLocked, params->SParams.PaletteEnabled);
		}
		palette->SetPaletteType(params->SParams.PaletteType);
		//
		// Restore manually assigned colors for locked pens from serialized params
		//
		if (cpcPalette) {
			cpcPalette->ApplyPenColors(params->SParams.PaletteColors);
		}
		//
		// Create a temporal bitmap to do all the process
		// We will create the temporal bitmap with a virtual size (result of width/height by pixel size)
		// TODO: Modes like EGX & co
		//
		// Target resolution is set in the struct by the UI Maybe the user changed / is using a custom resolution
		//
		std::shared_ptr<Image> resizedImage = GFXResize::GetResizeBitmap(srcImage, params->RParams);
		if (!resizedImage) {
			Log::Error("CPCBitmap::Convert resize failed (%dx%d -> %dx%d)", srcImage->GetWidth(), srcImage->GetHeight(), params->RParams.TargetWidth, params->RParams.TargetHeight);
			return;
		}
		//
		// Apply transparent color key if enabled (before quantization)
		// This marks pixels matching a specific color (e.g. magenta) as transparent
		//
		if (params->RParams.UseTransparentColor) {
			Log::Info(LogChannel::General, "[CPC] Applying transparent color key (R=%d G=%d B=%d tol=%d)", params->RParams.TransparentColorR, params->RParams.TransparentColorG,
					  params->RParams.TransparentColorB, params->RParams.TransparentColorTolerance);
			RgbColor keyColor(static_cast<uint8_t>(params->RParams.TransparentColorR), static_cast<uint8_t>(params->RParams.TransparentColorG),
							  static_cast<uint8_t>(params->RParams.TransparentColorB));
			GFXResize::MarkColorAsTransparent(resizedImage, keyColor, params->RParams.TransparentColorTolerance);
		}
		//
		// Now we have a working RGBA bitmap with the chosen
		// CPC resolution but in 32bpp
		//
		// Quantization & Dithering pass
		// Generate contrast table with color correction parameters
		//
		GFXColor::ContrastInitialize(params->CParams);
		//
		// Create quantization matrix.
		// For CPC Max 27 colors (CPC Palette),
		// For CPC+ should be 4096 available colors
		//
		// Number of lines is variable (not limited to 312 lines) so we allow to convert
		// any size bitmap (for further procesing with tiles or sprites)
		//
		int numColors = palette->GetSystemMaxColors();
		GFXQuantization::QuantizationInit(numColors, resizedImage->GetHeight());
		//
		// If the source image is paletized and UseSourcePalette is enabled, copy its
		// palette entries into the active pens before quantization runs, so the
		// quantization step maps to already-correct system colors with no guessing
		//
		GFXQuantization::ApplySourcePalette(palette, srcImage, params);
		//
		// Parameter KGauss on UI !?
		// if (prm.filtre)
		//     Palettiser(source, prm);
		// For Amstrad CPC, palette is fixed
		// For Amstrad CPC+, palette is RGB
		//

		//
		// Color quantization to the CPC palette
		// Dithering and smoothing applied if any
		//
		// prm.RParams.PixelSize = params->RParams.PixelSize;
		Log::Info(LogChannel::General, "[CPC] Quantizing and dithering (dither=%s pattern=%s smoothness=%s)...", params->DParams.Method.c_str(),
				  params->DParams.Pattern ? "on" : "off", params->QParams.Smoothness ? "on" : "off");
		GFXQuantization::ApplyQuantizationAndDither(palette, resizedImage, params);
		//
		// Calculate the number of colors detected on the image after the first processing
		//
		int nbCol = GFXQuantization::GetNumberColorsFound();
		Log::Info(LogChannel::General, "[CPC] Colors found after quantization: %d (max: %d)", nbCol, numColors);
		//
		// Color reduction to fit the max number of colors on CPC mode
		//
		int colSplit = 0;
		Log::Info(LogChannel::General, "[CPC] Reducing colors to fit mode palette...");
		GFXQuantization::ApplyColorReduction(palette, resizedImage, params, colSplit);
		//
		// Restore the transparent pen enabled state now that quantization is done
		//
		if (transPenSlot >= 0 && transPenSlot < (int)params->SParams.PaletteEnabled.size())
			params->SParams.PaletteEnabled[transPenSlot] = savedTransPenEnabled;
		Log::Info(LogChannel::General, "[CPC] Conversion complete (%dx%d native).", resizedImage->GetWidth(), resizedImage->GetHeight());
		//
		// Store native resolution before aspect correction
		// This is the actual CPC resolution (e.g., 160x200 for Mode 0)
		//
		nativeWidth = resizedImage->GetWidth();
		nativeHeight = resizedImage->GetHeight();
		//
		// Store the converted image (native CPC format, before preview transformations)
		// This is the quantized/dithered image at native resolution
		//
		convertedImage = resizedImage;
		//
		// Bitmap conversion
		// Bitmap creation from internal bytes
		//
		// Apply CPC screen aspect ratio correction and display effects
		// This scales the image to have correct square-pixel aspect ratio
		//
		CPCScreen::ScalingParams screenParams;
		screenParams.Mode = params->SParams.TargetMode;
		screenParams.ScaleFactor = previewScaleFactor;
		screenParams.Scanlines = previewScanlines;
		screenParams.ScanlineIntensity = 0.3f;
		//
		// Apply aspect correction if enabled (can be disabled for pixel-perfect view)
		//
		if (previewAspectCorrection) {
			previewImage = CPCScreen::ApplyAspectCorrection(resizedImage, screenParams);
			//
			// If aspect correction failed, fall back to original quantized image
			//
			if (!previewImage) {
				previewImage = resizedImage;
			}
		} else {
			//
			// No aspect correction: use the image generated directly
			//
			previewImage = resizedImage;
		}
	}
}
