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
#include <memory>
#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include <convert/convert.bitmap.params.h>
#include <convert/convert.palette.h>

namespace RetrodevLib {

	//
	// Quantization utilities and procedures
	//
	class GFXQuantization {
	public:
		//
		// Initializes the quantization table
		//
		// MaxColors: Maximum number of colors supported in the system
		// MaxLines: Maximum number of lines for the graphic to be quantizied
		//
		static void QuantizationInit(int MaxColors, int MaxLines);

		//
		// Quantization procedure plus dithering if required.
		// Bitmap must have logical size (multiplied by the pixel size).
		//
		// palette: Palette object
		// source: Source image
		// prm: Conversion parameters
		//
		static void ApplyQuantizationAndDither(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, const GFXParams* prm);

		//
		// Returns the number of colors found in the image after the
		// quantization / dithering / palette reduction process
		//
		static int GetNumberColorsFound();

		//
		// Color reduction to match the given palette max colors
		// source: input image
		// prm: conversion parameters
		// colSplit: returned split index (or similar)
		//
		static void ApplyColorReduction(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, const GFXParams* prm, int& colSplit);

		//
		// When UseSourcePalette is enabled and the source image is paletized, copies
		// the source palette entries that are actually used into the active pen slots.
		// If the number of used entries exceeds PaletteMaxColors(), a warning is logged
		// and extra entries are clamped.
		// sourceImage: the original (paletized) source image, NOT the resized copy
		// prm: conversion parameters
		//
		static void ApplySourcePalette(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> sourceImage, const GFXParams* prm);

	private:
		//
		// Holds the color hits per line collected during the conversion step.
		// For example, for the CPC we may have 27 colors and up to 312 lines.
		//
		static std::vector<std::vector<int>> colorsFound;
		//
		// Stores the configured maximum number of system colors
		//
		static int maxSystemColors;
		//
		// Stores the configured maximum number of lines
		//
		static int maxSystemLines;

		//
		// Fetch one pixel color adjusting with smoothness and dithering
		// palette: palette instance
		// source: source image
		// xPix, yPix: logical pixel coordinates
		// prm: conversion parameters
		// ditherAmount: dither matrix amount
		//
		static RgbColor GetPixelAdjusted(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, int xPix, int yPix, const GFXParams* prm, int ditherAmount);

		//
		// Find the best N colors in the palette to display the image.
		// Criteria: higher frequencies or larger distances depending on ReductionMethod.
		// Respects locked pens (color cannot be changed) and disabled pens (cannot be used)
		//
		static void FindColorMatching(std::shared_ptr<IPaletteConverter> palette, int maxPen, const GFXParams* prm);

		//
		// Convert using the standard algorithm and the selected per-line colors table
		//
		static void ConvertStd(std::shared_ptr<Image> source, const GFXParams* prm, std::shared_ptr<IPaletteConverter> palette, std::vector<std::vector<RgbColor>>& tabCol);
	};

} // namespace RetrodevLib
