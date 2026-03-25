// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- dithering methods (ordered matrices and error diffusion).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <assets/image/image.h>
#include <assets/image/image.color.h>
#include <convert/convert.bitmap.params.h>
#include "dithering.params.h"

namespace RetrodevLib {

	//
	// Dithering method name constants
	//
	namespace DitheringMethods {
		const std::string None = "None";
		const std::string FloydSteinberg = "Floyd-Steinberg (2x2)";
		const std::string Bayer1 = "Bayer 1 (2X2)";
		const std::string Bayer2 = "Bayer 2 (4x4)";
		const std::string Bayer3 = "Bayer 3 (4X4)";
		const std::string Ordered1 = "Ordered 1 (2x2)";
		const std::string Ordered2 = "Ordered 2 (3x3)";
		const std::string Ordered3 = "Ordered 3 (4x4)";
		const std::string Ordered4 = "Ordered 4 (8x8)";
		const std::string ZigZag1 = "ZigZag 1 (3x3)";
		const std::string ZigZag2 = "ZigZag 2 (4x3)";
		const std::string ZigZag3 = "ZigZag 3 (5x4)";
		const std::string CheckerboardHeavy = "Checkerboard Heavy (4x4)";
		const std::string CheckerboardLight = "Checkerboard Light (4x4)";
		const std::string CheckerboardAlternate = "Checkerboard Alternate (4x4)";
		const std::string DiagonalWave = "Diagonal Wave (2x4)";
		const std::string SparseVertical = "Sparse Vertical (2x3)";
		const std::string SparseHorizontal = "Sparse Horizontal (3x2)";
		const std::string CrossPattern = "Cross Pattern (4x4)";
		const std::string ClusterDots = "Cluster Dots (3x3)";
		const std::string GradientHorizontal = "Gradient Horizontal (2x4)";
		const std::string GradientDiagonal = "Gradient Diagonal (4x4)";
	}

	// =====================================================================================
	//  GFXDithering
	// =====================================================================================
	//
	//  Provides dithering matrix methods for image quantization on retro platforms.
	//  Each matrix defines threshold values used to distribute quantization error
	//  or apply ordered patterns when reducing color depth.
	//
	//  Dithering Matrices Reference
	//  ----------------------------
	//
	//  ERROR DIFFUSION
	//  ~~~~~~~~~~~~~~~
	//  Floyd-Steinberg (2x2)
	//    Classic error diffusion kernel. Distributes quantization error to
	//    neighboring pixels with weights 7/16, 3/16, 5/16, 1/16.
	//    Best for photographic images with smooth gradients.
	//
	//       +---+---+
	//       | 7 | 3 |    error weights / 16
	//       +---+---+
	//       | 5 | 1 |
	//       +---+---+
	//
	//  BAYER (ORDERED THRESHOLD)
	//  ~~~~~~~~~~~~~~~~~~~~~~~~~
	//  Bayer 1 (2x2)
	//    Smallest Bayer matrix. Visible crosshatch pattern.
	//    Best for very low resolution or 1-bit displays.
	//
	//       +---+---+
	//       | 1 | 3 |
	//       +---+---+
	//       | 4 | 2 |
	//       +---+---+
	//
	//  Bayer 2 (4x4)
	//    Classic 4x4 Bayer with 16 threshold levels. Good general-purpose
	//    ordered dither for retro hardware with limited palettes (CPC, MSX).
	//
	//       +----+----+----+----+
	//       |  0 | 12 |  3 | 15 |
	//       +----+----+----+----+
	//       |  8 |  4 | 11 |  7 |
	//       +----+----+----+----+
	//       |  2 | 14 |  1 | 13 |
	//       +----+----+----+----+
	//       | 10 |  6 |  9 |  5 |
	//       +----+----+----+----+
	//
	//  Bayer 3 (4x4)
	//    Variant 4x4 Bayer with a different threshold arrangement.
	//    Produces a shifted pattern. Useful for A/B comparison.
	//
	//  ORDERED (CLUSTERED DOT)
	//  ~~~~~~~~~~~~~~~~~~~~~~~
	//  Ordered 1 (2x2)   - Sequential 2x2 threshold. Simple dot-growth pattern.
	//                       Best for very small sprites or tile graphics.
	//  Ordered 2 (3x3)   - 9-level threshold in odd-sized grid. Avoids moire
	//                       patterns that even-sized matrices can produce.
	//  Ordered 3 (4x4)   - Standard 4x4 ordered dither. Good all-rounder for
	//                       retro platforms with 4- or 16-color palettes.
	//  Ordered 4 (8x8)   - Large 64-level threshold matrix. Smoothest gradients
	//                       of all ordered methods. Best for higher resolution output.
	//
	//  ZIGZAG
	//  ~~~~~~
	//  ZigZag 1 (3x3)    - Sparse diagonal scatter with zeros. Produces a
	//                       diagonal line pattern for stylistic/artistic effects.
	//
	//       +---+---+---+
	//       | 0 | 4 | 0 |
	//       +---+---+---+
	//       | 3 | 0 | 1 |    zeros = no dither adjustment
	//       +---+---+---+
	//       | 0 | 2 | 0 |
	//       +---+---+---+
	//
	//  ZigZag 2 (4x3)    - Non-square zigzag. Suits non-square pixel aspect
	//                       ratios (e.g. CPC Mode 0 where pixels are 2x wide).
	//  ZigZag 3 (5x4)    - Largest zigzag, very sparse. Open, airy dither
	//                       pattern for wide-pixel modes.
	//
	//  EXPERIMENTAL / ARTISTIC PATTERNS
	//  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//  Checkerboard Heavy (4x4)   - Strong alternating 1/16 pattern, high contrast
	//  Checkerboard Light (4x4)   - Subtle alternating 1/4 pattern, low contrast
	//  Checkerboard Alternate (4x4) - Medium alternating 1/8 pattern
	//  Diagonal Wave (2x4)        - Horizontal stripe dithering with wave effect
	//  Sparse Vertical (2x3)      - Asymmetric sparse pattern with vertical bias
	//  Sparse Horizontal (3x2)    - Asymmetric sparse pattern with horizontal bias
	//  Cross Pattern (4x4)        - Repeating diamond-like cross pattern
	//  Cluster Dots (3x3)         - Aggressive dither with high values (10-19)
	//  Gradient Horizontal (2x4)  - Sequential 0-7 gradient threshold horizontally
	//  Gradient Diagonal (4x4)    - Diagonal sweep pattern for halftone effects
	//
	//  Choosing the Right Method
	//  -------------------------
	//    Photos / natural images         -> Floyd-Steinberg
	//    Retro 4-color (CPC Mode 1)      -> Bayer 2 or Ordered 3
	//    Retro wide pixels (CPC Mode 0)  -> ZigZag 2 or ZigZag 3
	//    Smooth gradients, higher res    -> Ordered 4 (8x8)
	//    Tiny sprites / tiles            -> Bayer 1 or Ordered 1
	//    Artistic / stylistic            -> ZigZag or Experimental patterns
	//
	// =====================================================================================

	class GFXDithering {
	public:
		//
		// Active dithering matrix
		//
		static std::vector<std::vector<double>> DitherMatrix;

		//
		// Dithering methods and matrix list
		//
		static std::map<std::string, std::vector<std::vector<double>>> DitheringMatrixList;
		//
		// Ordered list of dithering method names (preserves insertion order)
		//
		static std::vector<std::string> DitheringMethodsList;

		//
		// Returns the list of Dithering methods to be presented on the UI
		//
		static std::vector<std::string> GetDitheringMethods();

		//
		// Sets the matrix for further processing dithering.
		// Returns the amount of dithering we need to apply.
		//
		static int SetDitherMatrix(const DitheringParams& values);

		//
		// Apply dither for the indicated pixel in the given image.
		// p is the original color, choix is the color after palettization.
		// In error diffusion mode, the quantization error (p - choix) is
		// spread to neighboring pixels using the active DitherMatrix weights.
		// In ordered mode, the matrix threshold is added to the pixel color.
		//
		static void DoDitherPixel(std::shared_ptr<Image> source, int xPix, int yPix, RgbColor p, RgbColor choix, const struct GFXParams* prms);

	private:
		// No dithering
		static const std::vector<std::vector<double>> none;
		// Error diffusion
		static const std::vector<std::vector<double>> floyd;
		// Bayer ordered threshold
		static const std::vector<std::vector<double>> bayer1;
		static const std::vector<std::vector<double>> bayer2;
		static const std::vector<std::vector<double>> bayer3;
		// Ordered clustered dot
		static const std::vector<std::vector<double>> ord1;
		static const std::vector<std::vector<double>> ord2;
		static const std::vector<std::vector<double>> ord3;
		static const std::vector<std::vector<double>> ord4;
		//
		// ZigZag diagonal scatter
		//
		static const std::vector<std::vector<double>> zigzag1;
		static const std::vector<std::vector<double>> zigzag2;
		static const std::vector<std::vector<double>> zigzag3;
		//
		// Experimental / artistic patterns
		//
		static const std::vector<std::vector<double>> checkerboard_heavy;
		static const std::vector<std::vector<double>> checkerboard_light;
		static const std::vector<std::vector<double>> checkerboard_alternate;
		static const std::vector<std::vector<double>> diagonal_wave;
		static const std::vector<std::vector<double>> sparse_vertical;
		static const std::vector<std::vector<double>> sparse_horizontal;
		static const std::vector<std::vector<double>> cross_pattern;
		static const std::vector<std::vector<double>> cluster_dots;
		static const std::vector<std::vector<double>> gradient_horizontal;
		static const std::vector<std::vector<double>> gradient_diagonal;
	};

}