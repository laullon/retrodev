// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Amstrad CPC screen -- mode definitions, resolution and pixel geometry.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <memory>
#include <string>

namespace RetrodevLib {

	//
	// CPC Screen aspect ratio correction and display effects
	// Handles the non-square pixel aspect ratio of the Amstrad CPC and applies CRT-like effects
	//
	class CPCScreen {
	public:
		//
		// Scaling parameters for CPC display correction
		//
		struct ScalingParams {
			//
			//
			// CPC video mode (Mode 0, Mode 1, Mode 2)
			//
			//
			// Affects pixel aspect correction. CPC pixels are non-square and require
			// integer scaling to map the native CPC resolution to a 4:3 PAL display
			// without fractional pixel artifacts.
			//
			// Examples (base integer scales that yield exact 4:3 output):
			// - Mode 0: 160x200 (40 chars * 4 pixels/char)
			//   Base scale: h=5, v=3 -> output 800x600 (4:3)
			// - Mode 1: 320x200 (40 chars * 8 pixels/char)
			//   Base scale: h=5, v=6 -> output 1600x1200 (4:3)
			// - Mode 2: 640x200 (80 chars * 8 pixels/char)
			//   Base scale: h=5, v=12 -> output 3200x2400 (4:3)
			//
			std::string Mode;
			//
			// Integer scale factor (1x, 2x, 3x, 4x, etc.)
			// Applied after aspect ratio correction
			//
			int ScaleFactor;
			//
			// Enable scanline effect (darkens every other line for CRT appearance)
			//
			bool Scanlines;
			//
			// Scanline intensity (0.0 = no effect, 1.0 = black lines)
			//
			float ScanlineIntensity;
			//
			// Default constructor
			//
			ScalingParams() : Mode("Mode 0"), ScaleFactor(2), Scanlines(false), ScanlineIntensity(0.3f) {}
		};
		//
		// Apply CPC aspect ratio correction and display effects
		// Takes a converted CPC image and returns a properly scaled version with correct aspect ratio
		// The output image has square pixels suitable for display on modern monitors
		//
		// source: Input image (CPC native resolution)
		// params: Scaling parameters (mode, scale factor, scanlines)
		// Returns: Scaled image with correct aspect ratio, or nullptr on error
		//
		static std::shared_ptr<Image> ApplyAspectCorrection(std::shared_ptr<Image> source, const ScalingParams& params);
		//
		// Get pixel aspect ratio correction factors for a given CPC mode
		// Returns raw integer scale factors that achieve a 4:3 display aspect ratio
		// hScale: horizontal scaling factor
		// vScale: vertical scaling factor
		//
		static void GetPixelAspectRatio(const std::string& mode, float& hScale, float& vScale);

	private:
		//
		// Apply scanline effect to the image
		// Darkens every other scanline to simulate CRT phosphor gaps
		//
		static void ApplyScanlines(std::shared_ptr<Image> image, float intensity);
	};

}
