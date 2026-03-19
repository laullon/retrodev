//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include "resize.params.h"

namespace RetrodevLib {

	ResizeParams::ResizeParams() {
		InterpMode = InterpolationMode::NearestNeighbor;
		ResMode = ScaleMode::Fit;
		TargetHeight = 0;
		TargetWidth = 0;
		SourceRect = {0, 0, 0, 0};
		//
		// Transparent color key defaults (disabled, magenta/pink as common chroma key)
		//
		UseTransparentColor = false;
		TransparentColorR = 255;
		TransparentColorG = 0;
		TransparentColorB = 255;
		TransparentColorTolerance = 0;
	}
} // namespace RetrodevLib