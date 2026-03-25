// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Project metadata -- resize settings serialisation.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <process/image/resize.params.h>
#include <glaze/glaze.hpp>

//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ResizeParams::ScaleMode> {
	using enum RetrodevLib::ResizeParams::ScaleMode;
	static constexpr auto value = enumerate("Fit", Fit, "Largest", Largest, "Smaller", Smaller, "Custom", Custom, "Original", Original);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ResizeParams::InterpolationMode> {
	using enum RetrodevLib::ResizeParams::InterpolationMode;
	static constexpr auto value =
		enumerate("NearestNeighbor", NearestNeighbor, "Low", Low, "Bicubic", Bicubic, "Bilinear", Bilinear, "High", High, "HighBicubic", HighBicubic, "HighBilinear", HighBilinear);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ResizeParams::Rectangle> {
	using T = RetrodevLib::ResizeParams::Rectangle;
	static constexpr auto value = object("X", &T::X, "Y", &T::Y, "Width", &T::Width, "Height", &T::Height);
};
//
// Glaze metadata for JSON serialization
//
template <> struct glz::meta<RetrodevLib::ResizeParams> {
	using T = RetrodevLib::ResizeParams;
	static constexpr auto value =
		object("ResMode", &T::ResMode, "InterpMode", &T::InterpMode, "TargetResolution", &T::TargetResolution, "TargetWidth", &T::TargetWidth, "TargetHeight", &T::TargetHeight,
			   "SourceRect", &T::SourceRect, "UseTransparentColor", &T::UseTransparentColor, "TransparentColorR", &T::TransparentColorR, "TransparentColorG", &T::TransparentColorG,
			   "TransparentColorB", &T::TransparentColorB, "TransparentColorTolerance", &T::TransparentColorTolerance, "TransparentPen", &T::TransparentPen);
};
