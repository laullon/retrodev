// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- resize and resampling pipeline.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include "resize.params.h"

namespace RetrodevLib {

	class GFXResize {
	public:
		/// <summary>
		/// Resizes one GFXBitmap passed as source into a newly created GFXBitmap with the given values
		/// All the resizing parameters are passed into the ResizeParams struct where the dimensions,
		/// resizing mode and interpolation modes are indicated
		/// </summary>
		/// <param name="resolution"></param>
		/// <param name="src"></param>
		/// <param name="values"></param>
		/// <returns></returns>
		static std::shared_ptr<Image> GetResizeBitmap(std::shared_ptr<Image> src, const ResizeParams& values);
		//
		// Returns the list of available scale modes
		//
		static std::vector<std::string> GetScaleModes();
		//
		// Returns the list of available interpolation modes
		//
		static std::vector<std::string> GetInterpolationModes();
		//
		// Mark a specific color as transparent (alpha = 0)
		// Useful for sprites/tiles with "chroma key" or "magic pink" backgrounds
		// tolerance: RGB distance threshold (0 = exact match only, 10-30 recommended for similar colors)
		//
		static void MarkColorAsTransparent(std::shared_ptr<Image> image, const RgbColor& keyColor, int tolerance = 0);

	private:
		//
		// Resize RGBA pixel data from src into dst using the given interpolation mode
		//
		static void ResizeRGBA(const uint8_t* src, int srcW, int srcH, int srcStride, uint8_t* dst, int dstW, int dstH, int dstStride, ResizeParams::InterpolationMode mode);
	};

}