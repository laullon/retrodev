//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "cpc.screen.h"
#include <convert/amstrad.cpc/amstrad.cpc.h>
#include <SDL3/SDL.h>

namespace RetrodevLib {

	//
	// Apply CPC aspect ratio correction and display effects
	// Scales the image to have correct square-pixel aspect ratio for modern displays
	//
	std::shared_ptr<Image> CPCScreen::ApplyAspectCorrection(std::shared_ptr<Image> source, const ScalingParams& params) {
		if (!source || !source->GetSurface())
			return nullptr;
		//
		// Get source dimensions
		//
		int srcWidth = source->GetWidth();
		int srcHeight = source->GetHeight();
		//
		// Calculate scaling to convert CPC non-square pixels to square pixels
		// CPC has non-square pixels with aspect ratios that include decimal heights
		// We use the ScaleFactor to multiply the base correction
		//
		float hScale, vScale;
		GetPixelAspectRatio(params.Mode, hScale, vScale);
		//
		// Apply user scale factor on top of aspect correction
		//
		hScale *= params.ScaleFactor;
		vScale *= params.ScaleFactor;
		//
		// Calculate destination dimensions
		//
		int dstWidth = (int)(srcWidth * hScale);
		int dstHeight = (int)(srcHeight * vScale);
		//
		// Create destination image
		//
		auto result = Image::ImageCreate(dstWidth, dstHeight);
		if (!result || !result->GetSurface())
			return nullptr;
		//
		// Lock both surfaces for raw pixel access; Pitch is taken from each accessor
		//
		auto srcAccess = source->LockPixels();
		auto dstAccess = result->LockPixels();
		const uint8_t* srcPixels = srcAccess.Pixels;
		int srcPitch = srcAccess.Pitch;
		uint8_t* dstPixels = dstAccess.Pixels;
		int dstPitch = dstAccess.Pitch;
		//
		// Perform scaling with floating-point precision using direct pixel pointer access
		//
		for (int y = 0; y < dstHeight; y++) {
			float srcYf = y / vScale;
			int srcY = (int)srcYf;
			if (srcY >= srcHeight)
				srcY = srcHeight - 1;
			const uint8_t* srcRow = srcPixels + srcY * srcPitch;
			uint8_t* dstRow = dstPixels + y * dstPitch;
			for (int x = 0; x < dstWidth; x++) {
				float srcXf = x / hScale;
				int srcX = (int)srcXf;
				if (srcX >= srcWidth)
					srcX = srcWidth - 1;
				//
				// Copy RGBA32 pixel directly as a single 32-bit value
				//
				*(uint32_t*)(dstRow + x * 4) = *(const uint32_t*)(srcRow + srcX * 4);
			}
		}
		//
		// Apply scanline effect if enabled
		//
		if (params.Scanlines) {
			ApplyScanlines(result, params.ScanlineIntensity);
		}
		//
		// Mark result as modified
		//
		result->MarkModified();
		return result;
	}

	//
	// Get pixel aspect ratio correction factors for CPC modes
	// CPC pixels are NOT square - they must be scaled to achieve 4:3 display aspect ratio
	//
	// CPC specs:
	// - Vertical: 312 total lines (200 visible), each character = 8 lines
	// - Horizontal varies by mode (character width differs)
	// - Displays on PAL 4:3 screen (720x576 pixels)
	//
	// Integer scaling technique:
	// To avoid fractional scaling artifacts, we scale to integer multiples that achieve
	// correct 4:3 aspect ratio, then display hardware scales down smoothly
	//
	void CPCScreen::GetPixelAspectRatio(const std::string& mode, float& hScale, float& vScale) {
		//
		// Calculate scaling factors to achieve 4:3 display aspect ratio
		// Formula: (width * hScale) : (height * vScale) = 4:3
		//
		if (mode == ConverterAmstradCPC::CPCModes::Mode0) {
			//
			// Mode 0: 160x200 (40 chars * 4 pixels/char)
			// Current aspect ratio: 160:200 = 4:5
			// To achieve 4:3: need (160*h)/(200*v) = 4/3
			// Solution: h=5, v=3 gives (160*5)/(200*3) = 800/600 = 4/3 ✓
			// Pixel aspect ratio: 5:3 (width is 5/3 times the height)
			//
			hScale = 5.0f;
			vScale = 3.0f;
		} else if (mode == ConverterAmstradCPC::CPCModes::Mode1) {
			//
			// Mode 1: 320x200 (40 chars * 8 pixels/char)
			// Current aspect ratio: 320:200 = 8:5
			// To achieve 4:3: need (320*h)/(200*v) = 4/3
			// Solution: h=5, v=6 gives (320*5)/(200*6) = 1600/1200 = 4/3 ✓
			// Pixel aspect ratio: 5:6 (width is 5/6 of the height - taller than wide)
			//
			hScale = 5.0f;
			vScale = 6.0f;
		} else {
			//
			// Mode 2: 640x200 (80 chars * 8 pixels/char)
			// Current aspect ratio: 640:200 = 16:5
			// To achieve 4:3: need (640*h)/(200*v) = 4/3
			// Solution: h=5, v=12 gives (640*5)/(200*12) = 3200/2400 = 4/3 ✓
			// Pixel aspect ratio: 5:12 (width is 5/12 of the height - very tall, narrow)
			//
			hScale = 5.0f;
			vScale = 12.0f;
		}
		//
		// Note: These integer scales produce large images (e.g., 800x600 for Mode 0)
		// which are then scaled down by display hardware for smooth rendering
		// This avoids fractional pixel artifacts common with direct decimal scaling
		//
	}

	//
	// Apply scanline effect to simulate CRT phosphor gaps
	// Darkens every other line to create the characteristic CRT look
	//
	void CPCScreen::ApplyScanlines(std::shared_ptr<Image> image, float intensity) {
		if (!image || !image->GetSurface())
			return;
		//
		// Clamp intensity to valid range
		//
		if (intensity < 0.0f)
			intensity = 0.0f;
		if (intensity > 1.0f)
			intensity = 1.0f;
		int width = image->GetWidth();
		int height = image->GetHeight();
		//
		// Lock surface for raw pixel access
		//
		auto pa = image->LockPixels();
		uint8_t* pixels = pa.Pixels;
		int pitch = pa.Pitch;
		float darkFactor = 1.0f - intensity;
		//
		// Darken every other scanline
		//
		for (int y = 1; y < height; y += 2) {
			uint8_t* row = pixels + y * pitch;
			for (int x = 0; x < width; x++) {
				uint8_t* pixel = row + x * 4;
				//
				// Skip transparent pixels
				//
				if (pixel[3] == 0)
					continue;
				//
				// Darken the pixel based on intensity
				//
				pixel[0] = static_cast<uint8_t>(pixel[0] * darkFactor);
				pixel[1] = static_cast<uint8_t>(pixel[1] * darkFactor);
				pixel[2] = static_cast<uint8_t>(pixel[2] * darkFactor);
			}
		}
	}
} // namespace RetrodevLib
