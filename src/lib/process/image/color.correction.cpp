// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Lib
//
// Image processing -- per-channel colour correction (brightness, contrast, saturation).
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include "color.correction.h"
#include <utils/utils.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace RetrodevLib {

	std::vector<uint8_t> GFXColor::tblContrast(256);
	int GFXColor::m1;
	int GFXColor::m2;

	void GFXColor::ContrastInitialize(const ColorCorrectionParams& prms) {
		double c = prms.Contrast / 100.0;
		for (int i = 0; i < 256; i++)
			tblContrast[i] = Utils::MinMaxByte(static_cast<float>(((((i / 255.0) - 0.5) * c) + 0.5) * 255.0));
		//
		// Palette reduction: parse lower limit string
		//
		if (prms.PaletteReductionLower == PaletteReductionLimits::Lower_0x11) {
			m1 = 0x11;
		} else if (prms.PaletteReductionLower == PaletteReductionLimits::Lower_0x22) {
			m1 = 0x22;
		} else if (prms.PaletteReductionLower == PaletteReductionLimits::Lower_0x33) {
			m1 = 0x33; // 0x11 | 0x22
		} else {
			m1 = 0x00; // None
		}
		//
		// Palette reduction: parse upper limit string
		//
		if (prms.PaletteReductionUpper == PaletteReductionLimits::Upper_0xEE) {
			m2 = 0xEE;
		} else if (prms.PaletteReductionUpper == PaletteReductionLimits::Upper_0xDD) {
			m2 = 0xDD;
		} else if (prms.PaletteReductionUpper == PaletteReductionLimits::Upper_0xCC) {
			m2 = 0xCC; // 0xEE & 0xDD
		} else {
			m2 = 0xFF; // None
		}
	}

	RgbColor GFXColor::ApplyColorCorrection(const ColorCorrectionParams& prm, RgbColor p) {
		//
		// Skip color correction for transparent pixels - return as-is
		//
		if (p.IsTransparent())
			return p;
		//
		// Reduce the amount of bits per component if needed
		//
		switch (prm.ColorBits) {
			//
			// 12 bits (4 bits per component)
			//
			case 12:
				p.r = (p.r >> 4) * 17;
				p.g = (p.g >> 4) * 17;
				p.b = (p.b >> 4) * 17;
				break;
			//
			// 9 bits (3 bits per component)
			//
			case 9:
				p.r = (p.r >> 5) * 36;
				p.g = (p.g >> 5) * 36;
				p.b = (p.b >> 5) * 36;
				break;
			//
			// 6 bits (2 bits per component)
			//
			case 6:
				p.r = (p.r >> 6) * 85;
				p.g = (p.g >> 6) * 85;
				p.b = (p.b >> 6) * 85;
				break;
		}
		//
		// Color cap filter with palette reduction masks
		//
		p.r = Utils::MinMaxByte(((p.r | m1) & m2) * prm.ColorCorrectionRed / 100.0f);
		p.g = Utils::MinMaxByte(((p.g | m1) & m2) * prm.ColorCorrectionGreen / 100.0f);
		p.b = Utils::MinMaxByte(((p.b | m1) & m2) * prm.ColorCorrectionBlue / 100.0f);
		//
		// Contrast, brightness and saturation adjustment
		//
		if (p.r != 0 || p.g != 0 || p.b != 0) {
			//
			// Take the corresponding value from the contrast table
			//
			float r = static_cast<float>(tblContrast[p.r]);
			float g = static_cast<float>(tblContrast[p.g]);
			float b = static_cast<float>(tblContrast[p.b]);
			if (prm.Brightness != 100 || prm.Saturation != 100) {
				SetLumiSat(prm.Brightness > 100 ? (100 + (prm.Brightness - 100) * 2) / 100.0F : prm.Brightness / 100.0F, prm.Saturation / 100.0F, r, g, b);
			}
			p.r = Utils::MinMaxByte(r);
			p.g = Utils::MinMaxByte(g);
			p.b = Utils::MinMaxByte(b);
		}
		return p;
	}

	void GFXColor::SetLumiSat(float lumi, float satur, float& r, float& g, float& b) {
		float min = std::min(r, std::min(g, b));
		float max = std::max(r, std::max(g, b));
		float dif = max - min;
		float hue = 0;
		//
		// Calculate hue (0-360 degrees, converted to 0-255 range)
		//
		if (max > min) {
			if (g == max)
				hue = (b - r) / dif * 60.0f + 120.0f;
			else if (b == max)
				hue = (r - g) / dif * 60.0f + 240.0f;
			else
				hue = (g - b) / dif * 60.0f + (b > g ? 360.0f : 0);
			if (hue < 0)
				hue = hue + 360.0f;
		}
		hue *= 255.0f / 360.0f;
		//
		// Calculate saturation (with safety check for division by zero)
		//
		float sat = 0;
		if (max > 0) {
			sat = satur * (dif / max) * 255.0f;
		}
		//
		// Apply brightness
		//
		float bri = lumi * max;
		r = g = b = bri;
		//
		// Convert back from HSV to RGB if saturation is non-zero
		//
		if (sat != 0) {
			float max_val = bri;
			dif = bri * sat / 255.0f;
			float min_val = bri - dif;
			float h = hue * 360.0f / 255.0f;
			if (h < 60.0f) {
				r = max_val;
				g = h * dif / 60.0f + min_val;
				b = min_val;
			} else if (h < 120.0f) {
				r = -(h - 120.0f) * dif / 60.0f + min_val;
				g = max_val;
				b = min_val;
			} else if (h < 180.0f) {
				r = min_val;
				g = max_val;
				b = (h - 120.0f) * dif / 60.0f + min_val;
			} else if (h < 240.0f) {
				r = min_val;
				g = -(h - 240.0f) * dif / 60.0f + min_val;
				b = max_val;
			} else if (h < 300.0f) {
				r = (h - 240.0f) * dif / 60.0f + min_val;
				g = min_val;
				b = max_val;
			} else if (h <= 360.0f) {
				r = max_val;
				g = min_val;
				b = -(h - 360.0f) * dif / 60.0f + min_val;
			} else {
				r = g = b = 0;
			}
		}
	}

	std::vector<std::string> GFXColor::GetPaletteReductionLowerLimits() {
		return {PaletteReductionLimits::Lower_None, PaletteReductionLimits::Lower_0x11, PaletteReductionLimits::Lower_0x22, PaletteReductionLimits::Lower_0x33};
	}

	std::vector<std::string> GFXColor::GetPaletteReductionUpperLimits() {
		return {PaletteReductionLimits::Upper_None, PaletteReductionLimits::Upper_0xEE, PaletteReductionLimits::Upper_0xDD, PaletteReductionLimits::Upper_0xCC};
	}

}
