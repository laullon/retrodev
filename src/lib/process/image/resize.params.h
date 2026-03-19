//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <string>

namespace RetrodevLib {
	//
	// Parameters used for image resizing operations
	// This struct is passed from the GUI into the resizing backend to control
	// dimensions, interpolation and pixel sizing.
	//
	struct ResizeParams {
		//
		// Mode of resizing
		//
		enum class ScaleMode { Fit, Largest, Smaller, Custom, Original } ResMode;
		//
		// Interpolation mode to be applied during resizing
		//
		enum class InterpolationMode {
			//
			// NearestNeighbor: nearest-neighbor sampling. Fastest, preserves hard pixel edges (good for pixel art).
			//
			NearestNeighbor,
			//
			// Low: low-quality box-like filter. Very fast, produces blockier / slightly blurred results.
			//
			Low,
			//
			// Bicubic: bicubic interpolation. Smooth results with more detail preservation than bilinear.
			//
			Bicubic,
			//
			// Bilinear: bilinear interpolation. Smooth and fast; good balance between quality and performance.
			//
			Bilinear,
			//
			// High: high-quality filtering (e.g. Catmull-Rom or similar). Prioritizes sharpness and detail.
			//
			High,
			//
			// HighBicubic: higher-quality bicubic variant (e.g. Mitchell-Netravali). Reduces ringing/artifacts.
			//
			HighBicubic,
			//
			// HighBilinear: enhanced bilinear (prefiltered/antialiased) offering better quality than plain bilinear.
			//
			HighBilinear
		} InterpMode;

		std::string TargetResolution;
		//
		// Target dimensions (defined by converter or user)
		//
		int TargetWidth;
		int TargetHeight;
		//
		// Source rectangle. If ScaleMode == User this rectangle defines the source
		// region to resample. Otherwise it will be derived from ScaleMode.
		//
		struct Rectangle {
			int X, Y, Width, Height;
		} SourceRect;
		//
		// Transparent color key (chroma key)
		// If enabled, all pixels matching this color (within tolerance) are marked as transparent
		//
		bool UseTransparentColor;
		int TransparentColorR;
		int TransparentColorG;
		int TransparentColorB;
		int TransparentColorTolerance;

		// Default constructor
		ResizeParams();
	};
	//
	// List of all available scale modes
	//
	static const std::vector<std::string> ScaleModeNames = {"Fit", "Largest", "Smaller", "Custom", "Original"};
	//
	// List of all available interpolation modes
	//
	static const std::vector<std::string> InterpolationModeNames = {"Nearest Neighbor", "Low", "Bicubic", "Bilinear", "High", "High Bicubic", "High Bilinear"};
} // namespace RetrodevLib
