// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Bitmap conversion parameters.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <convert/convert.params.h>
#include <process/image/color.correction.params.h>
#include <process/image/quantization.params.h>
#include <process/image/resize.params.h>
#include <process/image/dithering.params.h>

namespace RetrodevLib {

	struct GFXParams {
		ConvertParams SParams;
		ResizeParams RParams;
		DitheringParams DParams;
		QuantizationParams QParams;
		ColorCorrectionParams CParams;
	};
}
