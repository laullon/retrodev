//-----------------------------------------------------------------------------------------------------------------------
//
//
//
//
//
//
//
//-----------------------------------------------------------------------------------------------------------------------

#include <utils/utils.h>
#include <algorithm>
#include <climits>
#include <vector>
#include "quantization.h"
#include "dithering.h"
#include "color.correction.h"
#include <log/log.h>
#include <retrodev.lib.h>

namespace RetrodevLib {

	std::vector<std::vector<int>> GFXQuantization::colorsFound;
	int GFXQuantization::maxSystemColors = 0;
	int GFXQuantization::maxSystemLines = 0;

	//
	// Initializes the quantization table
	//
	void GFXQuantization::QuantizationInit(int MaxColors, int MaxLines) {
		Log::Info(LogChannel::General, "[Quantize] Init: %d colors, %d lines.", MaxColors, MaxLines);
		// Save the max colors & lines for further processing
		maxSystemColors = MaxColors;
		maxSystemLines = MaxLines;
		// Create the colors found table
		colorsFound.assign(MaxColors, std::vector<int>(MaxLines, 0));
	}

	void GFXQuantization::ApplyQuantizationAndDither(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, const GFXParams* prm) {
		//
		// Sanity Check: Look if the quantization has been initialized
		//
		if (colorsFound.empty() || !prm || !palette) {
			Log::Warning(LogChannel::General, "[Quantize] ApplyQuantizationAndDither skipped: not initialized or null params.");
			return;
		}
		//
		// Initialize dither matrix and retrieve the amount of dither to be applied
		// When pattern mode is active, we defer dithering to the pattern processing
		//
		int ditherAmount = prm->DParams.Pattern ? 0 : GFXDithering::SetDitherMatrix(prm->DParams);
		//
		// Used to store current processing pixel RGB color
		//
		RgbColor p(0, 0, 0);
		//
		// In case of paletized systems, hold the index to the palette and the RGB color
		//
		int paletteIndex = 0;
		RgbColor paletteColor(0, 0, 0);
		//
		// Process image line by line
		//
		for (int yPix = 0; yPix < source->GetHeight(); yPix++) {
			//
			// For all the horizontal pixels
			//
			for (int xPix = 0; xPix < source->GetWidth(); xPix++) {
				//
				// Fetch the adjusted pixel for this coordinate
				// When pattern is active, no dithering is applied yet (ditherAmount = 0)
				//
				p = GetPixelAdjusted(palette, source, xPix, yPix, prm, ditherAmount);
				//
				// Skip transparent pixels (don't quantize or add to histogram)
				//
				if (p.IsTransparent())
					continue;
				paletteIndex = palette->GetSystemIndexByColor(p, prm->SParams.ColorSelectionMode);
				paletteColor = palette->GetSystemColorByIndex(paletteIndex);
				//
				// Store color in histogram for current line
				//
				colorsFound[paletteIndex][yPix]++;
				//
				// If pattern mode is NOT active, store the pixel now
				//
				if (!prm->DParams.Pattern) {
					//
					// When dithering is active, p already contains the dithered color
					// When dithering is off, use exact palette color
					//
					if (ditherAmount > 0) {
						source->SetPixelColor(xPix, yPix, p);
					} else {
						source->SetPixelColor(xPix, yPix, paletteColor);
					}
				}
			}
		}
		//
		// Apply pattern dithering if required
		// Process pairs of scanlines to create alternating pattern
		//
		if (prm->DParams.Pattern) {
			for (int yPix = 0; yPix < source->GetHeight() - 1; yPix += 2) {
				for (int xPix = 0; xPix < source->GetWidth(); xPix++) {
					RgbColor n1(0, 0, 0);
					RgbColor n2(0, 0, 0);
					//
					// Get original pixels (no dithering was applied in first pass)
					//
					RgbColor p0 = source->GetPixelColor(xPix, yPix);
					RgbColor p1 = source->GetPixelColor(xPix, yPix + 1);
					//
					// Skip pattern dithering if either pixel is transparent
					//
					if (p0.IsTransparent() || p1.IsTransparent())
						continue;
					//
					// Apply color correction if needed
					//
					p0 = GFXColor::ApplyColorCorrection(prm->CParams, p0);
					p1 = GFXColor::ApplyColorCorrection(prm->CParams, p1);
					//
					// Average of two pixels
					//
					int r = (p0.r + p1.r);
					int g = (p0.g + p1.g);
					int b = (p0.b + p1.b);
					//
					// Create two colors by dividing the sum with different factors
					// PatternLow creates darker color, PatternHigh creates lighter color
					//
					uint8_t r1 = Utils::MinMaxByte(r / prm->DParams.PatternLow);
					uint8_t g1 = Utils::MinMaxByte(g / prm->DParams.PatternLow);
					uint8_t b1 = Utils::MinMaxByte(b / prm->DParams.PatternLow);
					RgbColor c1(r1, g1, b1);
					n2 = palette->GetSystemColorByIndex(palette->GetSystemIndexByColor(c1, prm->SParams.ColorSelectionMode));
					uint8_t r2 = Utils::MinMaxByte(r / prm->DParams.PatternHigh);
					uint8_t g2 = Utils::MinMaxByte(g / prm->DParams.PatternHigh);
					uint8_t b2 = Utils::MinMaxByte(b / prm->DParams.PatternHigh);
					RgbColor c2(r2, g2, b2);
					n1 = palette->GetSystemColorByIndex(palette->GetSystemIndexByColor(c2, prm->SParams.ColorSelectionMode));
					//
					// Alternate colors in checkerboard pattern
					//
					source->SetPixelColor(xPix, yPix, (xPix & 1) == 0 ? n1 : n2);
					source->SetPixelColor(xPix, yPix + 1, (xPix & 1) == 0 ? n2 : n1);
				}
			}
		}
	}

	RgbColor GFXQuantization::GetPixelAdjusted(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, int xPix, int yPix, const GFXParams* prm,
											   int ditherAmount) {
		RgbColor p(0, 0, 0);
		//
		// Sanity check
		//
		if (!prm || !palette)
			return p;
		//
		// Get the pixel at the current position
		//
		p = source->GetPixelColor(xPix, yPix);
		//
		// Skip processing for transparent pixels - return as-is
		//
		if (p.IsTransparent())
			return p;
		//
		// Apply smoothness if enabled
		// Use weighted averaging where center pixel has more influence
		//
		if (prm->QParams.Smoothness) {
			//
			// Start with center pixel weighted more heavily (weight = 4)
			//
			int r = p.r * 4;
			int g = p.g * 4;
			int b = p.b * 4;
			int totalWeight = 4;
			//
			// Sample only cardinal neighbors (up, down, left, right) with weight = 1
			//
			const int dx[] = {0, 0, -1, 1};
			const int dy[] = {-1, 1, 0, 0};
			for (int i = 0; i < 4; i++) {
				int nx = xPix + dx[i];
				int ny = yPix + dy[i];
				if (nx >= 0 && nx < source->GetWidth() && ny >= 0 && ny < source->GetHeight()) {
					RgbColor neighbor = source->GetPixelColor(nx, ny);
					r += neighbor.r;
					g += neighbor.g;
					b += neighbor.b;
					totalWeight++;
				}
			}
			//
			// Average with weights (center has 4x influence vs neighbors)
			//
			p.r = static_cast<uint8_t>(r / totalWeight);
			p.g = static_cast<uint8_t>(g / totalWeight);
			p.b = static_cast<uint8_t>(b / totalWeight);
		}
		//
		// Apply color correction to the pixel
		//
		p = GFXColor::ApplyColorCorrection(prm->CParams, p);
		//
		// If the amount of dither is greater than zero apply the dithering
		//
		if (ditherAmount > 0) {
			RgbColor c(0, 0, 0);
			int index = palette->GetSystemIndexByColor(p, prm->SParams.ColorSelectionMode);
			c = palette->GetSystemColorByIndex(index);
			GFXDithering::DoDitherPixel(source, xPix, yPix, p, c, prm);
			//
			// Read back the dithered pixel from the image since DoDitherPixel modifies the image
			// This ensures the histogram is built from actual dithered colors
			//
			p = source->GetPixelColor(xPix, yPix);
		}
		return p;
	}

	int GFXQuantization::GetNumberColorsFound() {
		if (colorsFound.empty())
			return 0;
		int nbCol = 0;
		for (size_t i = 0; i < colorsFound.size(); i++) {
			bool memoCol = false;
			for (size_t y = 0; y < colorsFound[i].size(); y++) {
				if (!memoCol && colorsFound[i][y] > 0) {
					nbCol++;
					memoCol = true;
				}
			}
		}
		return nbCol;
	}

	void GFXQuantization::ApplyColorReduction(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> source, const GFXParams* prm, int& colSplit) {
		UNUSED(colSplit);
		//
		// Sanity check
		//
		if (!prm || !palette) {
			Log::Warning(LogChannel::General, "[Quantize] ApplyColorReduction skipped: null params.");
			return;
		}
		//
		int MaxColors = palette->PaletteMaxColors();
		//
		// Create an array per line with the maximum allowed colors in display
		// Use actual image height since it's already resized
		//
		std::vector<std::vector<RgbColor>> linesCol(MaxColors, std::vector<RgbColor>(source->GetHeight()));
		//
		// Since there are modes where we could have different set of colors depending on vertical line
		// We fetch the max using the line as parameter
		//
		int maxPen = palette->PaletteMaxColors();
		FindColorMatching(palette, maxPen, prm);
		//
		// Reduce the image by the given max colors per line
		//
		for (int y = 0; y < source->GetHeight(); y++) {
			maxPen = palette->PaletteMaxColorsByLine(y);
			for (int i = 0; i < maxPen; i++)
				linesCol[i][y] = palette->PenGetColor(i);
		}
		ConvertStd(source, prm, palette, linesCol);
	}

	void GFXQuantization::FindColorMatching(std::shared_ptr<IPaletteConverter> palette, int maxPen, const GFXParams* prm) {
		if (colorsFound.empty() || !prm || !palette) {
			Log::Warning(LogChannel::General, "[Quantize] FindColorMatching skipped: not initialized or null params.");
			return;
		}
		int FindMax = maxSystemColors;
		//
		// Clear histogram for locked pens
		//
		for (int x = 0; x < maxPen; x++) {
			if (palette->PenGetLock(x)) {
				for (int y = 0; y < maxSystemLines; y++)
					colorsFound[palette->PenGetColorIndex(x)][y] = 0;
			}
		}
		if (prm->QParams.ReductionType == QuantizationParams::ReductionMethod::HigherFrequencies) {
			//
			// Assign all pens by frequency (most common colors first)
			//
			for (int pen = 0; pen < maxPen; pen++) {
				if (!palette->PenGetEnabled(pen) || palette->PenGetLock(pen))
					continue;
				int maxCount = 0;
				int bestColorIndex = 0;
				//
				// Find most frequent color in histogram
				//
				for (int i = 0; i < FindMax; i++) {
					int totalCount = 0;
					for (int y = 0; y < maxSystemLines; y++)
						totalCount += colorsFound[i][y];
					if (totalCount > maxCount) {
						maxCount = totalCount;
						bestColorIndex = i;
					}
				}
				//
				// No image colors remain — stop assigning pens to avoid wasting slots on black
				//
				if (maxCount == 0)
					break;
				//
				// Assign color to pen
				//
				palette->PenSetColorIndex(pen, bestColorIndex);
				//
				// Clear histogram for assigned color
				//
				for (int y = 0; y < maxSystemLines; y++)
					colorsFound[bestColorIndex][y] = 0;
			}
		} else {
			//
			// HigherDistances: First by frequency, then alternate between distance and frequency
			//
			int firstPen = -1;
			RgbColor firstColor;
			bool useDistance = false;
			for (int pen = 0; pen < maxPen; pen++) {
				if (!palette->PenGetEnabled(pen) || palette->PenGetLock(pen))
					continue;
				if (firstPen == -1) {
					//
					// First pen: always use frequency
					//
					int maxCount = 0;
					int bestColorIndex = 0;
					for (int i = 0; i < FindMax; i++) {
						int totalCount = 0;
						for (int y = 0; y < maxSystemLines; y++)
							totalCount += colorsFound[i][y];
						if (totalCount > maxCount) {
							maxCount = totalCount;
							bestColorIndex = i;
						}
					}
					//
					// No image colors remain — stop assigning pens to avoid wasting slots on black
					//
					if (maxCount == 0)
						break;
					palette->PenSetColorIndex(pen, bestColorIndex);
					firstColor = palette->GetSystemColorByIndex(bestColorIndex);
					firstPen = pen;
					//
					// Clear histogram
					//
					for (int y = 0; y < maxSystemLines; y++)
						colorsFound[bestColorIndex][y] = 0;
				} else {
					if (useDistance) {
						//
						// Find color with maximum distance from first color
						//
						int maxDist = 0;
						int bestColorIndex = 0;
						for (int i = 0; i < FindMax; i++) {
							int totalCount = 0;
							for (int y = 0; y < maxSystemLines; y++)
								totalCount += colorsFound[i][y];
							//
							// Only consider colors that are actually used
							//
							if (totalCount > 0) {
								RgbColor c = palette->GetSystemColorByIndex(i);
								int dist = palette->ColorDistance(c, firstColor);
								if (dist > maxDist) {
									maxDist = dist;
									bestColorIndex = i;
								}
							}
						}
						//
						// If no color found with distance > 0, fall back to frequency
						//
						if (maxDist == 0) {
							int maxCount = 0;
							for (int i = 0; i < FindMax; i++) {
								int totalCount = 0;
								for (int y = 0; y < maxSystemLines; y++)
									totalCount += colorsFound[i][y];
								if (totalCount > maxCount) {
									maxCount = totalCount;
									bestColorIndex = i;
								}
							}
							//
							// Histogram exhausted — no colors left to assign; stop
							//
							if (maxCount == 0)
								break;
						}
						palette->PenSetColorIndex(pen, bestColorIndex);
						//
						// Clear histogram
						//
						for (int y = 0; y < maxSystemLines; y++)
							colorsFound[bestColorIndex][y] = 0;
					} else {
						//
						// Use frequency
						//
						int maxCount = 0;
						int bestColorIndex = 0;
						for (int i = 0; i < FindMax; i++) {
							int totalCount = 0;
							for (int y = 0; y < maxSystemLines; y++)
								totalCount += colorsFound[i][y];
							if (totalCount > maxCount) {
								maxCount = totalCount;
								bestColorIndex = i;
							}
						}
						//
						// Histogram exhausted — no colors left to assign; stop
						//
						if (maxCount == 0)
							break;
						palette->PenSetColorIndex(pen, bestColorIndex);
						//
						// Clear histogram
						//
						for (int y = 0; y < maxSystemLines; y++)
							colorsFound[bestColorIndex][y] = 0;
					}
					//
					// Alternate between distance and frequency
					//
					useDistance = !useDistance;
				}
			}
		}
		//
		// Sort palette if requested
		//
		if (prm->QParams.SortPalette) {
			for (int x = 0; x < maxPen - 1; x++) {
				if (!palette->PenGetEnabled(x) || palette->PenGetLock(x))
					continue;
				for (int c = x + 1; c < maxPen; c++) {
					if (!palette->PenGetEnabled(c) || palette->PenGetLock(c))
						continue;
					if (palette->PenGetColorIndex(x) > palette->PenGetColorIndex(c)) {
						int tmp = palette->PenGetColorIndex(x);
						palette->PenSetColorIndex(x, palette->PenGetColorIndex(c));
						palette->PenSetColorIndex(c, tmp);
					}
				}
			}
		}
	}

	void GFXQuantization::ConvertStd(std::shared_ptr<Image> source, const GFXParams* prm, std::shared_ptr<IPaletteConverter> palette, std::vector<std::vector<RgbColor>>& tabCol) {
		//
		// Sanity check
		//
		if (!prm || !palette)
			return;
		//
		// Process each line in the already-resized image
		//
		for (int y = 0; y < source->GetHeight(); y++) {
			int maxPen = palette->PaletteMaxColorsByLine(y);
			//
			// Process each pixel in the line
			//
			for (int x = 0; x < source->GetWidth(); x++) {
				int minDistance = INT_MAX;
				RgbColor pixelColor = source->GetPixelColor(x, y);
				int selectedPen = 0;
				//
				// Skip transparent pixels - leave them as-is
				//
				if (pixelColor.IsTransparent())
					continue;
				//
				// Find closest color in palette
				// Skip disabled pens (cannot be used at all)
				// Locked pens CAN be used (they just can't have their color changed)
				//
				for (int penIndex = 0; penIndex < maxPen; penIndex++) {
					//
					// Skip disabled pens only (locked pens can still be used for rendering)
					//
					if (!palette->PenGetEnabled(penIndex))
						continue;
					RgbColor paletteColor = tabCol[penIndex][y];
					int distance = palette->ColorDistance(pixelColor, paletteColor);
					if (distance < minDistance) {
						selectedPen = penIndex;
						minDistance = distance;
						//
						// Early exit if exact match found
						//
						if (distance == 0)
							break;
					}
				}
				//
				// Set pixel to the final color
				//
				RgbColor finalColor = palette->PenGetColor(selectedPen);
				source->SetPixelColor(x, y, finalColor);
			}
		}
	}

	void GFXQuantization::ApplySourcePalette(std::shared_ptr<IPaletteConverter> palette, std::shared_ptr<Image> sourceImage, const GFXParams* prm) {
		//
		// Guard: only active when the flag is set and the source is a paletized image
		//
		if (!prm || !prm->QParams.UseSourcePalette || !sourceImage || !sourceImage->IsPaletized())
			return;
		int maxPens = palette->PaletteMaxColors();
		//
		// Scan pixel data to find which palette indices are actually used
		//
		bool seen[256] = {};
		{
			auto access = sourceImage->LockPixels();
			for (int y = 0; y < sourceImage->GetHeight(); y++) {
				const uint8_t* row = access.Pixels + y * access.Pitch;
				for (int x = 0; x < sourceImage->GetWidth(); x++)
					seen[row[x]] = true;
			}
		}
		//
		// Count how many distinct palette entries are used
		//
		int usedCount = 0;
		for (int i = 0; i < 256; i++)
			if (seen[i])
				usedCount++;
		//
		// Warn and clamp if the source uses more entries than the target mode supports
		//
		if (usedCount > maxPens)
			Log::Warning("GFXQuantization::ApplySourcePalette: source uses %d palette entries but target mode supports only %d -- clamping", usedCount, maxPens);
		//
		// Map each used source palette entry (in index order) to a pen slot
		//
		int pen = 0;
		for (int idx = 0; idx < 256 && pen < maxPens; idx++) {
			if (!seen[idx])
				continue;
			RgbColor color = sourceImage->GetPaletteColor(idx);
			int systemIdx = palette->GetSystemIndexByColor(color, prm->SParams.ColorSelectionMode);
			palette->PenSetColorIndex(pen, systemIdx);
			pen++;
		}
	}
} // namespace RetrodevLib
