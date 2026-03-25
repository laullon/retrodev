// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- dithering methods (ordered matrices and error diffusion).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include <algorithm>
#include <map>
#include <vector>
#include <string>
#include "dithering.h"
#include <utils/utils.h>

namespace RetrodevLib {

	const std::vector<std::vector<double>> GFXDithering::none = {};
	const std::vector<std::vector<double>> GFXDithering::floyd = {{7, 3}, {5, 1}};
	const std::vector<std::vector<double>> GFXDithering::bayer1 = {{1, 3}, {4, 2}};
	const std::vector<std::vector<double>> GFXDithering::bayer2 = {{0, 12, 3, 15}, {8, 4, 11, 7}, {2, 14, 1, 13}, {10, 6, 9, 5}};
	const std::vector<std::vector<double>> GFXDithering::bayer3 = {{1, 9, 3, 11}, {13, 5, 15, 7}, {4, 12, 2, 10}, {16, 8, 14, 6}};
	const std::vector<std::vector<double>> GFXDithering::ord1 = {{1, 3}, {2, 4}};
	const std::vector<std::vector<double>> GFXDithering::ord2 = {{8, 3, 4}, {6, 1, 2}, {7, 5, 9}};
	const std::vector<std::vector<double>> GFXDithering::ord3 = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
	const std::vector<std::vector<double>> GFXDithering::ord4 = {{0, 48, 12, 60, 3, 51, 15, 63},   {32, 16, 44, 28, 35, 19, 47, 31}, {8, 56, 4, 52, 11, 59, 7, 55},
																 {40, 24, 36, 20, 43, 27, 39, 23}, {2, 50, 14, 62, 1, 49, 13, 61},	 {34, 18, 46, 30, 33, 17, 45, 29},
																 {10, 58, 6, 54, 9, 57, 5, 53},	   {42, 26, 38, 22, 41, 25, 37, 21}};
	const std::vector<std::vector<double>> GFXDithering::zigzag1 = {{0, 4, 0}, {3, 0, 1}, {0, 2, 0}};
	const std::vector<std::vector<double>> GFXDithering::zigzag2 = {{0, 4, 2, 0}, {6, 0, 5, 3}, {0, 7, 1, 0}};
	const std::vector<std::vector<double>> GFXDithering::zigzag3 = {{0, 0, 0, 7, 0}, {0, 2, 6, 9, 8}, {3, 0, 1, 5, 0}, {0, 4, 0, 0, 0}};
	const std::vector<std::vector<double>> GFXDithering::checkerboard_heavy = {{1.0, 16.0, 1.0, 16.0}, {16.0, 1.0, 16.0, 1.0}, {1, 16, 1, 16}, {16, 1, 16, 1}};
	const std::vector<std::vector<double>> GFXDithering::checkerboard_light = {{1, 4, 1, 4}, {4, 1, 4, 1}, {1, 4, 1, 4}, {4, 1, 4, 1}};
	const std::vector<std::vector<double>> GFXDithering::checkerboard_alternate = {{8, 1, 8, 1}, {1, 8, 1, 8}, {8, 1, 8, 1}, {1, 8, 1, 8}};
	const std::vector<std::vector<double>> GFXDithering::diagonal_wave = {{8, 16, 16, 8}, {16, 8, 8, 16}};
	const std::vector<std::vector<double>> GFXDithering::sparse_vertical = {{0, 3}, {0, 5}, {7, 1}};
	const std::vector<std::vector<double>> GFXDithering::sparse_horizontal = {{0, 0, 7}, {3, 5, 1}};
	const std::vector<std::vector<double>> GFXDithering::cross_pattern = {{1, 9, 4, 7}, {4, 7, 1, 9}, {1, 9, 4, 7}, {4, 7, 1, 9}};
	const std::vector<std::vector<double>> GFXDithering::cluster_dots = {{12, 11, 0}, {13, 10, 19}, {11, 13, 0}};
	const std::vector<std::vector<double>> GFXDithering::gradient_horizontal = {{3, 7, 6, 2}, {5, 4, 1, 0}};
	const std::vector<std::vector<double>> GFXDithering::gradient_diagonal = {{1, 5, 10, 14}, {3, 7, 8, 12}, {13, 9, 6, 2}, {15, 11, 4, 0}};

	std::vector<std::vector<double>> GFXDithering::DitherMatrix;
	std::map<std::string, std::vector<std::vector<double>>> GFXDithering::DitheringMatrixList = {
		{DitheringMethods::None, none},
		{DitheringMethods::FloydSteinberg, floyd},
		{DitheringMethods::Bayer1, bayer1},
		{DitheringMethods::Bayer2, bayer2},
		{DitheringMethods::Bayer3, bayer3},
		{DitheringMethods::Ordered1, ord1},
		{DitheringMethods::Ordered2, ord2},
		{DitheringMethods::Ordered3, ord3},
		{DitheringMethods::Ordered4, ord4},
		{DitheringMethods::ZigZag1, zigzag1},
		{DitheringMethods::ZigZag2, zigzag2},
		{DitheringMethods::ZigZag3, zigzag3},
		{DitheringMethods::CheckerboardHeavy, checkerboard_heavy},
		{DitheringMethods::CheckerboardLight, checkerboard_light},
		{DitheringMethods::CheckerboardAlternate, checkerboard_alternate},
		{DitheringMethods::DiagonalWave, diagonal_wave},
		{DitheringMethods::SparseVertical, sparse_vertical},
		{DitheringMethods::SparseHorizontal, sparse_horizontal},
		{DitheringMethods::CrossPattern, cross_pattern},
		{DitheringMethods::ClusterDots, cluster_dots},
		{DitheringMethods::GradientHorizontal, gradient_horizontal},
		{DitheringMethods::GradientDiagonal, gradient_diagonal},
	};

	//
	// Ordered list of methods (preserves insertion order, not alphabetical)
	//
	std::vector<std::string> GFXDithering::DitheringMethodsList = {DitheringMethods::None,
																   DitheringMethods::FloydSteinberg,
																   DitheringMethods::Bayer1,
																   DitheringMethods::Bayer2,
																   DitheringMethods::Bayer3,
																   DitheringMethods::Ordered1,
																   DitheringMethods::Ordered2,
																   DitheringMethods::Ordered3,
																   DitheringMethods::Ordered4,
																   DitheringMethods::ZigZag1,
																   DitheringMethods::ZigZag2,
																   DitheringMethods::ZigZag3,
																   DitheringMethods::CheckerboardHeavy,
																   DitheringMethods::CheckerboardLight,
																   DitheringMethods::CheckerboardAlternate,
																   DitheringMethods::DiagonalWave,
																   DitheringMethods::SparseVertical,
																   DitheringMethods::SparseHorizontal,
																   DitheringMethods::CrossPattern,
																   DitheringMethods::ClusterDots,
																   DitheringMethods::GradientHorizontal,
																   DitheringMethods::GradientDiagonal};

	std::vector<std::string> GFXDithering::GetDitheringMethods() {
		return DitheringMethodsList;
	}

	int GFXDithering::SetDitherMatrix(const DitheringParams& values) {
		//
		// "None" method always disables dithering regardless of percentage
		//
		if (values.Method == DitheringMethods::None) {
			DitherMatrix.clear();
			return 0;
		}
		int pct = values.Percentage << 1;
		//
		// Check if method exists in the list
		//
		if (DitheringMatrixList.count(values.Method)) {
			DitherMatrix = DitheringMatrixList[values.Method];
			//
			// If matrix is empty or percentage is 0, clear and return 0
			//
			if (DitherMatrix.empty() || pct <= 0) {
				DitherMatrix.clear();
				return 0;
			}
			//
			// Calculate sum and normalize matrix by percentage
			//
			double sum = 0;
			for (const auto& row : DitherMatrix) {
				for (double val : row) {
					sum += val;
				}
			}
			//
			// Avoid division by zero (shouldn't happen with valid matrices)
			//
			if (sum > 0) {
				for (auto& row : DitherMatrix) {
					for (double& val : row) {
						val = (val * pct) / sum;
					}
				}
			} else {
				DitherMatrix.clear();
				pct = 0;
			}
		} else {
			//
			// Method not found, disable dithering
			//
			DitherMatrix.clear();
			pct = 0;
		}
		return pct;
	}

	void GFXDithering::DoDitherPixel(std::shared_ptr<Image> source, int xPix, int yPix, RgbColor p, RgbColor choix, const GFXParams* prms) {
		if (DitherMatrix.empty() || !prms)
			return;
		//
		// Skip dithering for transparent pixels
		//
		if (p.IsTransparent())
			return;
		//
		// Apply error diffusion dithering
		// Distributes quantization error to neighboring pixels according to the matrix
		//
		if (prms->DParams.ErrorDiffusion) {
			for (size_t y = 0; y < DitherMatrix.size(); ++y) {
				for (size_t x = 0; x < DitherMatrix[y].size(); ++x) {
					//
					// Calculate target pixel position
					// Since image is already resized, work directly on pixel coordinates
					//
					int efX = xPix + static_cast<int>(x);
					int efY = yPix + static_cast<int>(y);
					if (efX < source->GetWidth() && efY < source->GetHeight()) {
						RgbColor pix = source->GetPixelColor(efX, efY);
						//
						// Don't diffuse error to transparent pixels
						//
						if (pix.IsTransparent())
							continue;
						//
						// Apply weighted error diffusion
						//
						pix.r = Utils::MinMaxByte(static_cast<double>(pix.r) + (p.r - choix.r) * DitherMatrix[y][x] / 256.0);
						pix.g = Utils::MinMaxByte(static_cast<double>(pix.g) + (p.g - choix.g) * DitherMatrix[y][x] / 256.0);
						pix.b = Utils::MinMaxByte(static_cast<double>(pix.b) + (p.b - choix.b) * DitherMatrix[y][x] / 256.0);
						source->SetPixelColor(efX, efY, pix);
					}
				}
			}
		} else {
			//
			// Apply ordered dithering
			// Uses the matrix as a threshold pattern
			//
			size_t xm = xPix % DitherMatrix[0].size();
			size_t ym = yPix % DitherMatrix.size();
			p.r = Utils::MinMaxByte(static_cast<double>(p.r) + DitherMatrix[ym][xm]);
			p.g = Utils::MinMaxByte(static_cast<double>(p.g) + DitherMatrix[ym][xm]);
			p.b = Utils::MinMaxByte(static_cast<double>(p.b) + DitherMatrix[ym][xm]);
			//
			// Write modified pixel back to image
			//
			source->SetPixelColor(xPix, yPix, p);
		}
	}

}